#pragma once

#include "donde/definitions.h"
#include "donde/feature_search/api.h"

// #include "spdlog/spdlog.h"

#include <iostream>
#include <unordered_map>

using namespace std;

namespace donde_toolkits {

namespace feature_search {

namespace search_manager {

class Worker {
  public:
    virtual std::string GetWorkerID() = 0;

    virtual uint64 GetFreeSpace() = 0;

    // ListShards report all shards this worker is serving.
    virtual std::vector<DBShard> ListShards() = 0;

    // ServeShard let the worker serve this shard, for its features' CRUD
    virtual RetCode ServeShard(const DBShard& shard_info) = 0;

    // CloseShard close db_id/shard_id.
    virtual RetCode CloseShard(const std::string& db_id, const std::string& shard_id) = 0;

    // AddFeatures to db_id/shard_id, delegate to remote worker.
    virtual std::vector<std::string> AddFeatures(const std::string& db_id,
                                                 const std::string& shard_id,
                                                 const std::vector<Feature>& fts)
        = 0;

    // Search feature in the worker. worker can have multiple dbs, multiple shards.
    // only search in the requested db.
    virtual std::vector<FeatureSearchItem> SearchFeature(const std::string& db_id,
                                                         const Feature& query, int topk)
        = 0;
};

using WorkerPtr = shared_ptr<Worker>;

} // namespace search_manager
} // namespace feature_search
} // namespace donde_toolkits
