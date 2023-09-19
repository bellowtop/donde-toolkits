#pragma once

#include "donde/definitions.h"
#include "donde/feature_search/driver.h"
#include "donde/feature_search/shard.h"
#include "shard_factory.h"
#include "shard_manager.h"

#include <string>
#include <unordered_map>
#include <vector>



namespace donde_toolkits ::feature_search ::search_manager {

class ShardManagerImpl : public ShardManager {

  public:
    ShardManagerImpl(Driver& driver, ShardFactory& factory);
    ~ShardManagerImpl() = default;

    std::tuple<Shard*, bool> FindOrCreateWritableShard(std::string db_id,
                                                       uint64 fts_count) override;

    // LoadShards from driver
    void LoadShards(std::string db_id) override;

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
    ShardFactory& _shard_factory;
};

} // namespace donde_toolkits::feature_search::search_manager
