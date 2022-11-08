#pragma once

#include "search/definitions.h"
#include "types.h"
#include "utils.h"

#include <string>
#include <vector>

class Worker {
  public:
    virtual std::string GetWorkerID() = 0;

    virtual uint64 GetFreeSpace() = 0;

    // ServeShard let the worker serve this shard, for its features' CRUD
    virtual RetCode ServeShard(const search::DBShard& shard_info) = 0;

    // CloseShard close db_id/shard_id.
    virtual RetCode CloseShard(const std::string& db_id, const std::string& shard_id) = 0;

    // AddFeatures to db_id/shard_id, delegate to remote worker.
    virtual RetCode AddFeatures(const std::string& db_id, const std::string& shard_id,
                                const std::vector<Feature>& fts)
        = 0;

    // Search feature in the worker. worker can have multiple dbs, multiple shards.
    // only search in the requested db.
    virtual std::vector<Feature> SearchFeature(const std::string& db_id, const Feature& query,
                                               int topk)
        = 0;
};
