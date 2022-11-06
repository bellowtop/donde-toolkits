#pragma once

#include "search/driver.h"
#include "types.h"
#include "utils.h"
#include "worker_api.h"

#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

const size_t DEFAULT_SHARD_CAPACITY = 1024 * 1024 * 1024;

class Shard {

  public:
    Shard(std::string db_id, std::string worker_id, size_t cap = DEFAULT_SHARD_CAPACITY,
          bool writable = false)
        : _shard_id(generate_uuid()),
          _db_id(db_id),
          _worker_id(worker_id),
          _capacity(cap),
          _used(0),
          _writable(writable){};

    // AddFeatures to this shard
    RetCode AddFeatures(std::vector<Feature> fts);

    RetCode AssignWorker(Worker* worker);

    RetCode Close();

  public:
    std::string _shard_id;
    std::string _db_id;

    std::string _worker_id;
    Worker* _worker = nullptr;

    size_t _capacity;
    size_t _used;
    bool _writable;
};

class ShardManager {

  public:
    ShardManager(search::Driver& driver) : _driver(driver){};

    Shard* FindOrCreateWritableShard(std::string db_id);

    RetCode LoadShards();

    // FIXME why shared_ptr? what the purpose of this api?
    std::vector<std::shared_ptr<search::DBItem>> ListUserDBs();

    // FIXME why poiter? what the purpose of this api?
    std::vector<Shard*> ListShards(std::string db_id);

  public:
    std::unordered_map<std::string, std::vector<Shard*>> db_shards;

    search::Driver& _driver;
};
