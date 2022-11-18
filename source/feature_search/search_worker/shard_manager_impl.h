#pragma once

#include "donde/definitions.h"
#include "donde/feature_search/api.h"
#include "donde/feature_search/search_worker/api.h"
#include "donde/utils.h"

#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

namespace donde {
namespace feature_search {
namespace search_worker {

class ShardManagerImpl : public ShardManager {

  public:
    ShardManagerImpl(Driver& driver);
    ~ShardManagerImpl() = default;

    std::vector<Shard*> ListShards(std::string db_id) override;

    RetCode ManageShard(std::string db_id, Shard*) override;

    RetCode CloseShard(std::string db_id, std::string shard_id) override;

  private:
    std::unordered_map<std::string, std::vector<Shard*>> _db_shards;

    std::unordered_map<std::string, DBItem> _user_dbs;

    Driver& _driver;
};

} // namespace search_worker
} // namespace feature_search
} // namespace donde
