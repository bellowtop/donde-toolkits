#include "search_manager/worker_client.h"

#include "gen/pb-cpp/feature_search_inner.grpc.pb.h"
#include "gen/pb-cpp/feature_search_inner.pb.h"
#include "grpcpp/create_channel.h"
#include "grpcpp/security/credentials.h"
#include "poco/RunnableAdapter.h"
#include "types.h"

#include <chrono>
#include <cstdlib>
#include <grpcpp/client_context.h>
#include <grpcpp/support/status.h>
#include <grpcpp/support/stub_options.h>
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

WorkerClient::WorkerClient(const std::string& addr) : _worker_id(generate_uuid()), _addr(addr) {
    auto ret = Connect();

    if (ret == RetCode::RET_OK) {
        // 1. get initial system info;
        auto pair = WorkerClient::get_system_info(_stub.get(), 10);
        auto status = pair.first;
        auto response = pair.second;

        if (status.ok()) {
            _worker_info = response.worker_info();
        }

        // 2. then check regulaly.
        Poco::RunnableAdapter<WorkerClient> ra(*this, &WorkerClient::check_liveness_loop);
        _liveness_check_thread.start(ra);
    }
};

// Connect to remote addr, and regularly check liveness.
RetCode WorkerClient::Connect() {
    spdlog::info("connected to remote worker");

    _conn = grpc::CreateChannel(_addr, grpc::InsecureChannelCredentials());
    _stub = FeatureSearchWorker::NewStub(_conn);

    return RetCode::RET_OK;
};

RetCode WorkerClient::DisConnect() {
    spdlog::info("disconnected to remote worker");

    stopped = true;

    //// !!!!call this join will crash doctest case!!!!
    // _liveness_check_thread.join();

    return RetCode::RET_OK;
};

uint64 WorkerClient::GetFreeSpace() {
    size_t total_used = 0;
    for (auto it = _served_shards.begin(); it != _served_shards.end(); it++) {
        total_used += it->second.capacity;
    };
    return _worker_info.capacity() - total_used;
};

// ServeShard let the worker serve this shard, for its features' CRUD
RetCode WorkerClient::ServeShard(const search::DBShard& shard_info) {
    auto found = _served_shards.find(shard_info.shard_id);
    if (found != _served_shards.end()) {
        spdlog::warn("shard {} is already served by this worker!", shard_info.shard_id);
        return RetCode::RET_OK;
    }

    auto free_space = GetFreeSpace();
    if (shard_info.capacity > free_space) {
        spdlog::info("this worker doesn't has enough space for the shard {} (worker free:{}, shard "
                     "capacity want: {}) !",
                     shard_info.shard_id, free, shard_info.capacity);
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

RetCode WorkerClient::CloseShard(const std::string& db_id, const std::string& shard_id) {

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
RetCode WorkerClient::AddFeatures(const std::string& db_id, const std::string& shard_id,
                                  const std::vector<Feature>& fts) {
    auto found = _served_shards.find(shard_id);
    if (found == _served_shards.end()) {
        spdlog::info("shard {} is not served by this worker!", shard_id);
        return RetCode::RET_ERR;
    }
    if (found->second.is_closed) {
        spdlog::info("shard {} is closed, cannot AddFeatures.!", shard_id);
        return RetCode::RET_ERR;
    }

    grpc::ClientContext ctx;
    BatchAddFeaturesRequest request;
    BatchAddFeaturesResponse response;

    _stub->BatchAddFeatures(&ctx, request, &response);

    return {};
};

std::vector<Feature> WorkerClient::SearchFeature(const std::string& db_id, const Feature& query,
                                                 int topk) {
    return {};
};

void WorkerClient::check_liveness_loop() {
    while (true) {
        // use channel select?
        if (stopped) {
            spdlog::info("in check liveness loop, got stopped, break the loop");
            break;
        }

        auto pair = WorkerClient::get_system_info(_stub.get(), 10);
        auto status = pair.first;
        auto response = pair.second;

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

std::pair<grpc::Status, GetSystemInfoResponse> WorkerClient::get_system_info(WorkerStub* stub,
                                                                             int timeout) {
    grpc::ClientContext ctx;

    if (timeout > 0) {
        auto now = std::chrono::steady_clock::now();
        auto deadline = now + std::chrono::seconds(timeout);
        ctx.set_deadline(deadline);
    }

    const GetSystemInfoRequest request{};
    GetSystemInfoResponse response{};
    auto status = stub->GetSystemInfo(&ctx, request, &response);

    return {status, response};
}
