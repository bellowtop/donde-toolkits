#pragma once

#include "donde/definitions.h"
#include "donde/feature_search/api.h"
#include "donde/feature_search/driver.h"
#include "donde/feature_search/search_worker/shard.h"

// #include "spdlog/spdlog.h"

#include <iostream>
#include <unordered_map>

using namespace std;

namespace donde_toolkits {

namespace feature_search {

namespace search_worker {

class ShardManager {

  public:
    ShardManager(Driver&){};

    virtual ~ShardManager() = default;

    virtual std::vector<Shard*> ListShards(std::string db_id) = 0;

    virtual RetCode ManageShard(std::string db_id, Shard*) = 0;

    virtual RetCode CloseShard(std::string db_id, std::string shard_id) = 0;

    virtual Shard* FindShard(std::string db_id) = 0;
};
;

} // namespace search_worker
} // namespace feature_search
} // namespace donde_toolkits
