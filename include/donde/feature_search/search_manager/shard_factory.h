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

class Shard;
class ShardManager;

class ShardFactory {
  public:
    virtual ~ShardFactory() = default;
    virtual Shard* CreateShard(ShardManager* mgr, DBShard shard_info) = 0;
};

} // namespace search_manager
} // namespace feature_search
} // namespace donde_toolkits
