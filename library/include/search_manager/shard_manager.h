#pragma once

#include "search/definitions.h"
#include "search/driver.h"
#include "search_manager/shard.h"
#include "search_manager/worker_api.h"
#include "types.h"
#include "utils.h"

#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

class ShardManager {

  public:
    ShardManager(search::Driver& driver);
    ~ShardManager() = default;

    std::tuple<Shard*, bool> FindOrCreateWritableShard(std::string db_id, uint64 fts_count);

    RetCode AssignWorkerToShard(Shard* shard, Worker* worker);

    std::vector<search::DBItem> ListUserDBs();

    std::vector<Shard*> ListShards(std::string db_id);

    RetCode CloseShard(std::string db_id, std::string shard_id);

    std::string CreateShard(search::DBShard shard_info);

    RetCode UpdateShard(search::DBShard shard_info);

  private:
    RetCode load_db_shards();

  private:
    std::unordered_map<std::string, std::vector<Shard*>> _db_shards;

    std::unordered_map<std::string, search::DBItem> _user_dbs;

    search::Driver& _driver;
};
