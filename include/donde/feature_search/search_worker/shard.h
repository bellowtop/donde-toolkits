#pragma once

#include "donde/definitions.h"
#include "donde/feature_search/definitions.h"

// #include "spdlog/spdlog.h"

#include <iostream>
#include <unordered_map>

using namespace std;

namespace donde_toolkits ::feature_search ::search_worker {

class ShardManager;

class Shard {

  public:
    Shard(ShardManager&, DBShard){};

    virtual ~Shard() = default;

    // Load features from driver; load index if needed.
    virtual void Load() = 0;

    // Start the loop, for add/search etc.
    virtual void Start() = 0;
    // Stop the loop.
    virtual void Stop() = 0;

    // AddFeatures to this shard
    virtual std::vector<std::string> AddFeatures(const std::vector<FeatureDbItem>& fts) = 0;

    // RemoveFeatures from this shard
    virtual RetCode RemoveFeatures(const std::vector<std::string>& feature_ids) = 0;

    // SearchFeature in this shard, delegate to worker client to do the actual search.
    virtual std::vector<FeatureSearchItem> SearchFeature(const Feature& query, int topk) = 0;

    // Close this shard, cannot add features from this shard, but still can search.
    virtual RetCode Close() = 0;

    // IsClosed return true if shard is closed.
    virtual bool IsClosed() = 0;

    // IsStopped return true if shard loop is stopped. a stopped shard means it will not respond to
    // any api calls (add/search etc.)
    virtual bool IsStopped() = 0;

    // Quick methods
    virtual std::string GetShardID() = 0;
    virtual DBShard GetShardInfo() = 0;
};

} // namespace donde_toolkits::feature_search::search_worker
