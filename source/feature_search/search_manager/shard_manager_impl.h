#pragma once

#include "donde/definitions.h"
#include "donde/feature_search/api.h"
#include "donde/feature_search/search_manager/api.h"
#include "donde/utils.h"

#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

namespace donde {
namespace feature_search {
namespace search_manager {

class ShardManagerImpl : public ShardManager {

  public:
    ShardManagerImpl(Driver& driver, ShardFactory* factory);
    ~ShardManagerImpl() = default;

    std::tuple<Shard*, bool> FindOrCreateWritableShard(std::string db_id,
                                                       uint64 fts_count) override;

    RetCode AssignWorkerToShard(Shard* shard, Worker* worker) override;

    std::vector<DBItem> ListUserDBs() override;

    std::vector<Shard*> ListShards(std::string db_id) override;

    RetCode CloseShard(std::string db_id, std::string shard_id) override;

    std::string CreateShard(DBShard shard_info) override;

    RetCode UpdateShard(DBShard shard_info) override;

  private:
    RetCode load_db_shards();

  private:
    std::unordered_map<std::string, std::vector<Shard*>> _db_shards;

    std::unordered_map<std::string, DBItem> _user_dbs;

    Driver& _driver;
    std::shared_ptr<ShardFactory> _shard_factory;
};

} // namespace search_manager
} // namespace feature_search
} // namespace donde
