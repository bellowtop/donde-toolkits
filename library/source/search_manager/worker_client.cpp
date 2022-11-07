#include "search_manager/worker_client.h"

#include "poco/RunnableAdapter.h"
#include "types.h"

#include <chrono>
#include <thread>

WorkerClient::WorkerClient(const std::string& addr) : _worker_id(generate_uuid()), _addr(addr) {
    auto ret = Connect();
    if (ret == RetCode::RET_OK) {
        // then check regulaly.
        Poco::RunnableAdapter<WorkerClient> ra(*this, &WorkerClient::check_liveness_loop);
        _liveness_check_thread.start(ra);
    }
};

// Connect to remote addr, and regularly check liveness.
RetCode WorkerClient::Connect() {
    spdlog::info("connected to remote worker");
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

        std::cout << ("in check liveness loop, sleep 1s") << std::endl;
        spdlog::info("in check liveness loop, sleep 1s");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}
