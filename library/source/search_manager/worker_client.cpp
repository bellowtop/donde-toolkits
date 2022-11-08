#include "search_manager/worker_client.h"

#include "grpcpp/create_channel.h"
#include "grpcpp/security/credentials.h"
#include "poco/RunnableAdapter.h"
#include "types.h"

#include <chrono>
#include <cstdlib>
#include <grpcpp/client_context.h>
#include <grpcpp/support/status.h>
#include <grpcpp/support/stub_options.h>
#include <thread>

using com::sekirocc::feature_search::inner::FeatureSearchWorker;

using com::sekirocc::feature_search::inner::GetSystemInfoRequest;
using com::sekirocc::feature_search::inner::GetSystemInfoResponse;

WorkerClient::WorkerClient(const std::string& addr) : _worker_id(generate_uuid()), _addr(addr) {
    auto ret = Connect();

    // then check regulaly.
    if (ret == RetCode::RET_OK) {
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

RetCode WorkerClient::CloseShard(const std::string& db_id, const std::string& shard_id) {
    return {};
};

// WorkerAPI implement
RetCode WorkerClient::AddFeatures(const std::string& db_id, const std::string& shard_id,
                                  const std::vector<Feature>& fts) {
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

        grpc::ClientContext ctx;
        auto now = std::chrono::steady_clock::now();
        auto timeout = now + std::chrono::seconds(10);
        ctx.set_deadline(timeout);

        const GetSystemInfoRequest request{};
        GetSystemInfoResponse response{};

        _liveness_check_time = now;

        auto status = _stub->GetSystemInfo(&ctx, request, &response);
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
