#pragma once

#include "search/definitions.h"
#include "search/driver.h"
#include "search_manager/worker_api.h"
#include "types.h"
#include "utils.h"

#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

class ShardManager;

class Shard {

  public:
    Shard(ShardManager* manager, search::DBShard shard_info)
        : _shard_info(shard_info), _shard_id(shard_info.shard_id), _db_id(shard_info.db_id){};

    ~Shard() = default;

    // Assign a worker for this shard.
    RetCode AssignWorker(Worker* worker);

    // AddFeatures to this shard, delegate to worker client to do the actual storage.
    RetCode AddFeatures(std::vector<Feature> fts);

    RetCode Close();

    // check the shard has been assigned worker or not.
    inline bool HasWorker() { return _worker == nullptr; };

    // check the shard is closed or not.
    inline bool IsClosed() { return _shard_info.is_closed; };

    inline std::string GetShardID() { return _shard_id; };

    inline search::DBShard GetShardInfo() { return _shard_info; };

  private:
    search::DBShard _shard_info;

    std::string _shard_id;
    std::string _db_id;

    std::string _worker_id;
    Worker* _worker = nullptr;

    ShardManager* _shard_mgr = nullptr;
};

class ShardManager {

  public:
    ShardManager(search::Driver& driver);
    ~ShardManager() = default;

    std::tuple<Shard*, bool> FindOrCreateWritableShard(std::string db_id, uint64 fts_count);

    std::vector<search::DBItem> ListUserDBs();

    std::vector<Shard*> ListShards(std::string db_id);

    RetCode CloseShard(std::string db_id, std::string shard_id);

    std::string CreateShard(search::DBShard shard_info);

  private:
    RetCode load_db_shards();

  private:
    std::unordered_map<std::string, std::vector<Shard*>> _db_shards;

    std::unordered_map<std::string, search::DBItem> _user_dbs;

    search::Driver& _driver;
};
