#pragma once

#include "types.h"
#include "utils.h"

#include <string>
#include <vector>

class Worker {
    virtual std::string GetWorkerID() = 0;

    // AddFeatures to db_id/shard_id.
    virtual RetCode AddFeatures(const std::string& db_id, const std::string& shard_id,
                                const std::vector<Feature>& fts)
        = 0;


    // Search feature in the worker. worker can have multiple dbs, multiple shards.
    // only search in the requested db.
    virtual std::vector<Feature> SearchFeature(const std::string& db_id, const Feature& query,
                                               int topk)
        = 0;
};
