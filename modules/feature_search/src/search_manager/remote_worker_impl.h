#pragma once

#include "donde/feature_search/worker.h"

namespace donde_toolkits ::feature_search ::search_manager {

class RemoteWorkerImpl : public Worker {

  public:
    RemoteWorkerImpl();
    ~RemoteWorkerImpl();

    std::string GetWorkerID() override;
    std::string GetAddress() override;

    bool Ready() override;

    uint64 GetFreeSpace() override;

    // ListShards report all shards this worker is serving.
    std::vector<DBShard> ListShards() override;

    // ServeShard let the worker serve this shard, for its features' CRUD
    RetCode ServeShard(Shard& shard) override;

    // CloseShard close db_id/shard_id.
    RetCode CloseShard(const std::string& db_id, const std::string& shard_id) override;

    // AddFeatures to db_id/shard_id, delegate to remote worker.
    std::vector<std::string> AddFeatures(const std::string& db_id, const std::string& shard_id,
                                         const std::vector<Feature>& fts) override;

    // Search feature in the worker. worker can have multiple dbs, multiple shards.
    // only search in the requested db.
    std::vector<FeatureSearchItem> SearchFeature(const std::string& db_id, const Feature& query,
                                                 int topk) override;
};
} // namespace donde_toolkits::feature_search::search_manager
