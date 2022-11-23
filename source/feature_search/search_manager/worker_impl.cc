#include "worker_impl.h"

#include "Poco/RunnableAdapter.h"
#include "api/feature_search_inner.grpc.pb.h"
#include "api/feature_search_inner.pb.h"
#include "grpcpp/create_channel.h"
#include "grpcpp/security/credentials.h"

#include <chrono>
#include <cstdlib>
#include <grpcpp/client_context.h>
#include <grpcpp/support/status.h>
#include <grpcpp/support/stub_options.h>
#include <numeric>
#include <openvino/runtime/properties.hpp>
#include <thread>
#include <utility>

using com::sekirocc::feature_search::inner::FeatureSearchWorker;

using com::sekirocc::feature_search::inner::GetSystemInfoRequest;
using com::sekirocc::feature_search::inner::GetSystemInfoResponse;

using com::sekirocc::feature_search::inner::BatchAddFeaturesRequest;
using com::sekirocc::feature_search::inner::BatchAddFeaturesResponse;

using com::sekirocc::feature_search::inner::ServeDBShardsRequest;
using com::sekirocc::feature_search::inner::ServeDBShardsResponse;

using com::sekirocc::feature_search::inner::CloseDBShardsRequest;
using com::sekirocc::feature_search::inner::CloseDBShardsResponse;

using com::sekirocc::feature_search::inner::GetSystemInfoRequest;
using com::sekirocc::feature_search::inner::GetSystemInfoResponse;

using com::sekirocc::feature_search::inner::SearchFeatureRequest;
using com::sekirocc::feature_search::inner::SearchFeatureResponse;

namespace donde {
namespace feature_search {
namespace search_manager {

WorkerImpl::WorkerImpl(const std::string& addr) : _worker_id(generate_uuid()), _addr(addr) {
    auto ret = Connect();

    if (ret == RetCode::RET_OK) {
        // 1. get initial system info;
        auto [status, response] = WorkerImpl::get_system_info(_stub.get(), 10);
        if (status.ok()) {
            _worker_info = response.worker_info();
        }

        // 2. then check regulaly.
        Poco::RunnableAdapter<WorkerImpl> ra(*this, &WorkerImpl::check_liveness_loop);
        _liveness_check_thread.start(ra);
    }
};

// Connect to remote addr, and regularly check liveness.
RetCode WorkerImpl::Connect() {
    spdlog::info("connected to remote worker");

    _conn = grpc::CreateChannel(_addr, grpc::InsecureChannelCredentials());
    _stub = FeatureSearchWorker::NewStub(_conn);

    return RetCode::RET_OK;
};

RetCode WorkerImpl::DisConnect() {
    spdlog::info("disconnected to remote worker");

    stopped = true;

    //// !!!!call this join will crash doctest case!!!!
    _liveness_check_thread.join();

    return RetCode::RET_OK;
};

uint64 WorkerImpl::GetFreeSpace() {
    uint64 total_used = 0;
    for (auto it = _served_shards.begin(); it != _served_shards.end(); it++) {
        total_used += it->second.capacity;
    };
    // another approach?
    // std::reduce(_served_shards.begin(), _served_shards.end(), 0,
    //             [](int sum, auto it) { return sum + it->second.capacity; });

    return _worker_info.capacity() - total_used;
};

std::vector<DBShard> WorkerImpl::ListShards() {
    std::vector<DBShard> shards;
    for (auto it = _served_shards.begin(); it != _served_shards.end(); it++) {
        shards.push_back(it->second);
    }
    return shards;
};

// ServeShard let the worker serve this shard, for its features' CRUD
RetCode WorkerImpl::ServeShard(const DBShard& shard_info) {
    auto found = _served_shards.find(shard_info.shard_id);
    if (found != _served_shards.end()) {
        spdlog::warn("shard {} is already served by this worker!", shard_info.shard_id);
        return RetCode::RET_OK;
    }

    auto free_space = GetFreeSpace();
    if (shard_info.capacity > free_space) {
        spdlog::info("this worker doesn't has enough space for the shard {} (worker free:{}, shard "
                     "capacity want: {}) !",
                     shard_info.shard_id, free_space, shard_info.capacity);
        return RetCode::RET_ERR;
    }

    grpc::ClientContext ctx;
    ServeDBShardsRequest request;
    ServeDBShardsResponse response;
    auto shard = request.add_shards();
    shard->set_db_id(shard_info.db_id);
    shard->set_shard_id(shard_info.shard_id);
    auto status = _stub->ServeDBShards(&ctx, request, &response);
    if (status.ok()) {
        _served_shards[shard_info.shard_id] = shard_info;
        return RetCode::RET_OK;
    }

    return RetCode::RET_ERR;
};

RetCode WorkerImpl::CloseShard(const std::string& db_id, const std::string& shard_id) {

    grpc::ClientContext ctx;
    CloseDBShardsRequest request;
    CloseDBShardsResponse response;
    auto shard = request.add_shards();
    shard->set_db_id(db_id);
    shard->set_shard_id(shard_id);
    auto status = _stub->CloseDBShards(&ctx, request, &response);
    if (status.ok()) {
        _served_shards[shard_id].is_closed = true;
        return RetCode::RET_OK;
    }

    return RetCode::RET_ERR;
};

// WorkerAPI implement
std::vector<std::string> WorkerImpl::AddFeatures(const std::string& db_id,
                                                 const std::string& shard_id,
                                                 const std::vector<Feature>& fts) {
    auto found = _served_shards.find(shard_id);
    if (found == _served_shards.end()) {
        spdlog::info("shard {} is not served by this worker!", shard_id);
        return {};
    }
    if (found->second.is_closed) {
        spdlog::info("shard {} is closed, cannot AddFeatures.!", shard_id);
        return {};
    }

    grpc::ClientContext ctx;
    BatchAddFeaturesRequest request;
    BatchAddFeaturesResponse response;

    request.set_db_id(db_id);
    request.set_shard_id(shard_id);
    for (auto& ft : fts) {
        auto item = request.add_feature_items();
        auto feat = item->mutable_feature();
        auto meta = item->mutable_meta();
        // TODO
    }

    auto status = _stub->BatchAddFeatures(&ctx, request, &response);
    if (status.ok()) {
        std::vector<std::string> feature_ids;
        auto ids = response.feature_ids();
        for (auto& id : ids) {
            feature_ids.push_back(id);
        }
        return feature_ids;
    }

    return {};
};

std::vector<FeatureSearchItem> WorkerImpl::SearchFeature(const std::string& db_id,
                                                         const Feature& query, int topk) {

    grpc::ClientContext ctx;
    SearchFeatureRequest request;
    SearchFeatureResponse response;

    request.set_topk(topk);
    request.add_db_ids()->assign(db_id);
    auto feat = request.mutable_query();
    // TODO

    auto status = _stub->SearchFeature(&ctx, request, &response);
    if (status.ok()) {
        std::vector<FeatureSearchItem> fts;
        for (auto item : response.items()) {
            com::sekirocc::common::FaceFeature one = item.feature();
            const std::string& one_blob = one.blob();
            donde::Feature ft(convertFeatureBlobToFloats(one_blob), std::string(one.model()),
                              one.version());

            float score = item.score();

            fts.emplace_back(std::move(ft), score);
        }
        return fts;
    }

    return {};
};

void WorkerImpl::check_liveness_loop() {
    while (true) {
        // use channel select?
        if (stopped) {
            spdlog::info("in check liveness loop, got stopped, break the loop");
            break;
        }

        auto [status, response] = WorkerImpl::get_system_info(_stub.get(), 10);

        // if (status.error_code() == grpc::Status::CANCELLED.error_code()) {
        if (!status.ok()) {
            spdlog::info("in check liveness loop, status is not ok, {}", status.error_message());
            _liveness = false;
        } else {
            _liveness = true;
        }

        spdlog::info("in check liveness loop, sleep 1s");

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

std::tuple<grpc::Status, GetSystemInfoResponse> WorkerImpl::get_system_info(WorkerStub* stub,
                                                                            int timeout) {
    grpc::ClientContext ctx;

    if (timeout > 0) {
        auto now = std::chrono::system_clock::now();
        auto deadline = now + std::chrono::seconds(timeout);
        ctx.set_deadline(deadline);
    }

    const GetSystemInfoRequest request{};
    GetSystemInfoResponse response{};
    auto status = stub->GetSystemInfo(&ctx, request, &response);

    return {status, response};
}

} // namespace search_manager
} // namespace feature_search
} // namespace donde
