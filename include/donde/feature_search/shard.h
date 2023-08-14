#pragma once

#include "donde/definitions.h"
#include "donde/feature_search/definitions.h"

// #include "spdlog/spdlog.h"

#include <iostream>
#include <unordered_map>

namespace donde_toolkits ::feature_search ::search_manager {

class Worker;
class ShardManager;
class Shard {

  public:
    virtual ~Shard() = default;

    virtual void Start() = 0;
    virtual void Stop() = 0;

    // Assign a worker for this shard.
    virtual RetCode AssignWorker(Worker* worker) = 0;

    // AddFeatures to this shard, delegate to worker client to do the actual storage.
    virtual std::vector<std::string> AddFeatures(const std::vector<Feature>& fts) = 0;

    // SearchFeature in this shard, delegate to worker client to do the actual search.
    virtual std::vector<FeatureSearchItem> SearchFeature(const Feature& query, int topk) = 0;

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

using ShardPtr = std::shared_ptr<Shard>;

} // namespace donde_toolkits::feature_search::search_manager
