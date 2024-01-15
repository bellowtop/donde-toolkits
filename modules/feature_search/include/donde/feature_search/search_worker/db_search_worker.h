#pragma once

#include "donde/definitions.h"
#include "donde/feature_search/definitions.h"

// #include "faiss/Index2Layer.h"

#include <map>
#include <memory>
#include <opencv2/core/hal/interface.h>



using json = nlohmann::json;

namespace donde_toolkits ::feature_search ::search_worker {

class ISearchWorker {

  public:
    // Searcher contructor, owns the input ShardManager pointer.
    // By the way, this is just an api, the implementation should owns the pointer.
    ISearchWorker(){};

    virtual ~ISearchWorker() = default;

    virtual RetCode Start() = 0;

    virtual RetCode Stop() = 0;

    // TrainIndex is an oppotunity for shards to train index, if they need to do that.
    virtual RetCode TrainIndex() = 0;

    // ServeShards let this searcher serve these shards, typically loads shards' data into search
    // engine.
    virtual RetCode ServeShards(const std::vector<DBShard>& shard_infos) = 0;

    // CloseShards close these shards, so that user cannot write to them.
    // if implementation map db with shard as 1:1, then after close the shard
    // user should call `ServeShards` for those db, so that there will be active writing shard
    // available, if not, user cannot AddFeatures to the db anymore.
    virtual RetCode CloseShards(const std::vector<DBShard>& shard_infos) = 0;

    // AddFeatures to db, actually to related shards.
    // if db map to multiple shards, then implementation should choose which shard to write.
    virtual std::vector<std::string> AddFeatures(const std::string& db_id,
                                                 const std::vector<FeatureDbItem>& features)
        = 0;

    virtual RetCode RemoveFeatures(const std::string& db_id,
                                   const std::vector<std::string>& feature_ids)
        = 0;

    virtual std::vector<FeatureSearchItem> SearchFeature(const std::string& db_id,
                                                         const Feature& query, size_t topk)
        = 0;
};

} // namespace donde_toolkits::feature_search::search_worker
