#pragma once

#include "gen/pb-cpp/feature_search_inner.grpc.pb.h"
#include "search_manager/worker_api.h"
#include "types.h"
#include "utils.h"

#include <Poco/Thread.h>
#include <chrono>
#include <grpcpp/channel.h>
#include <string>
#include <vector>

using WorkerStub = com::sekirocc::feature_search::inner::FeatureSearchWorker::Stub;

class WorkerClient : public Worker {
  public:
    WorkerClient(const std::string& addr);
    virtual ~WorkerClient() = default;

    // Connect to remote addr, and regularly check liveness.
    RetCode Connect();
    RetCode DisConnect();

    inline std::string GetWorkerID() override { return _worker_id; };

    // CloseShard close db_id/shard_id.
    RetCode CloseShard(const std::string& db_id, const std::string& shard_id) override;

    // WorkerAPI implement
    RetCode AddFeatures(const std::string& db_id, const std::string& shard_id,
                        const std::vector<Feature>& fts) override;

    std::vector<Feature> SearchFeature(const std::string& db_id, const Feature& query,
                                       int topk) override;

  private:
    void check_liveness_loop();

  private:
    Poco::Thread _liveness_check_thread;

    std::chrono::time_point<chrono::steady_clock> _liveness_check_time;
    bool _liveness;

    bool stopped = false;

    std::string _worker_id;
    std::string _addr;

    std::shared_ptr<grpc::Channel> _conn;
    std::unique_ptr<WorkerStub> _stub;

    bool _connected;

    bool _is_writing;

    std::vector<std::string> _shard_ids;

    size_t _capacity;
    size_t _used;
};
