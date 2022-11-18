#pragma once

#include "donde/definitions.h"
#include "donde/feature_search/api.h"

// #include "spdlog/spdlog.h"

#include <iostream>
#include <unordered_map>

using namespace std;

namespace donde {

namespace feature_search {

namespace search_worker {

class Shard {

  public:
    virtual ~Shard() = default;

    virtual void Start() = 0;
    virtual void Stop() = 0;

    // AddFeatures to this shard
    virtual RetCode AddFeatures(const std::vector<Feature>& fts) = 0;

    // SearchFeature in this shard, delegate to worker client to do the actual search.
    virtual std::vector<FeatureScore> SearchFeature(const Feature& query, int topk) = 0;

    virtual RetCode Close() = 0;

    // check the shard has been assigned worker or not.
    virtual bool HasWorker() = 0;

    // check the shard is closed or not.
    virtual bool IsClosed() = 0;

    // check the shard is closed or not.
    virtual bool IsStopped() = 0;

    virtual std::string GetShardID() = 0;

    virtual DBShard GetShardInfo() = 0;
};

} // namespace search_worker
} // namespace feature_search
} // namespace donde
