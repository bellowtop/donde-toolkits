#pragma once

#include "donde/definitions.h"
#include "donde/feature_search/api.h"

// #include "spdlog/spdlog.h"

#include <iostream>
#include <unordered_map>

using namespace std;

namespace donde {

namespace feature_search {

namespace search_manager {

// Coordinator & Reducer
class Coordinator {
  public:
    virtual ~Coordinator() = default;

    virtual void Start() = 0;

    virtual void Stop() = 0;

    // std::vector<WorkerPtr> ListWorkers() = 0;

    virtual std::vector<DBItem> ListUserDBs() = 0;

    // AddFeatures to this db, we need find proper shard to store these fts.
    virtual RetCode AddFeatures(const std::string& db_id, const std::vector<Feature>& fts) = 0;

    // SearchFeatures in this db.
    virtual std::vector<FeatureScore> SearchFeature(const std::string& db_id, const Feature& query,
                                                    int topk)
        = 0;
};

} // namespace search_manager
} // namespace feature_search
} // namespace donde
