#pragma once

#include "donde/definitions.h"
#include "donde/feature_search/driver.h"
#include "shard.h"

#include <iostream>
#include <unordered_map>


namespace donde_toolkits ::feature_search ::search_worker {

class ShardManager {

  public:
    ShardManager(Driver&){};

    virtual ~ShardManager() = default;

    virtual std::vector<Shard*> ListShards(std::string db_id) = 0;

    virtual RetCode ManageShard(std::string db_id, Shard*) = 0;

    virtual RetCode CloseShard(std::string db_id, std::string shard_id) = 0;

    virtual Shard* FindShard(std::string db_id) = 0;
};

} // namespace donde_toolkits::feature_search::search_worker
