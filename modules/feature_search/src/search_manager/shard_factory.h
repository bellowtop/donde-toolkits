#pragma once

#include "donde/feature_search/definitions.h"

#include <iostream>
#include <unordered_map>



namespace donde_toolkits ::feature_search ::search_manager {

class Shard;
class ShardManager;

class ShardFactory {
  public:
    virtual ~ShardFactory() = default;
    virtual Shard* CreateShard(ShardManager* mgr, DBShard shard_info) = 0;
};

} // namespace donde_toolkits::feature_search::search_manager
