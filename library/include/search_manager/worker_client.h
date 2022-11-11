#pragma once

#include "gen/pb-cpp/feature_search_inner.grpc.pb.h"
#include "gen/pb-cpp/feature_search_inner.pb.h"
#include "search_manager/worker_api.h"
#include "types.h"
#include "utils.h"

#include <Poco/Thread.h>
#include <chrono>
#include <grpcpp/channel.h>
#include <string>
#include <vector>

using WorkerStub = com::sekirocc::feature_search::inner::FeatureSearchWorker::Stub;

using com::sekirocc::feature_search::inner::GetSystemInfoRequest;
using com::sekirocc::feature_search::inner::GetSystemInfoResponse;

using com::sekirocc::feature_search::inner::WorkerInfo;

class WorkerClient : public Worker {
  public:
    WorkerClient(const std::string& addr);
    virtual ~WorkerClient() = default;

    // Connect to remote addr, and regularly check liveness.
    RetCode Connect();
    RetCode DisConnect();

    inline std::string GetWorkerID() override { return _worker_id; };

    uint64 GetFreeSpace() override;

    // ListShards list all shards served by this worker
    std::vector<search::DBShard> ListShards() override;

    // ServeShard let the worker serve this shard, for its features' CRUD
    RetCode ServeShard(const search::DBShard& shard_info) override;

    // CloseShard close db_id/shard_id.
    RetCode CloseShard(const std::string& db_id, const std::string& shard_id) override;

    // WorkerAPI implement
    RetCode AddFeatures(const std::string& db_id, const std::string& shard_id,
                        const std::vector<Feature>& fts) override;

    // SearchFeature search query in this db, across all shards.
    std::vector<FeatureScore> SearchFeature(const std::string& db_id, const Feature& query,
                                            int topk) override;

  private:
    void check_liveness_loop();

    static std::tuple<grpc::Status, GetSystemInfoResponse> get_system_info(WorkerStub* stub,
                                                                           int timeout = 0);

  private:
    Poco::Thread _liveness_check_thread;

    WorkerInfo _worker_info;

    std::unordered_map<std::string, search::DBShard> _served_shards;

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
};
