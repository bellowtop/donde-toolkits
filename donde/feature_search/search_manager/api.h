#pragma once

#include "donde/definitions.h"
#include "donde/feature_search/api.h"

// #include "spdlog/spdlog.h"

#include <iostream>
#include <unordered_map>

using namespace std;

namespace donde {

namespace feature_search {

namespace search_manager {

// Coordinator & Reducer
class Coordinator {
  public:
    virtual ~Coordinator() = default;

    virtual void Start() = 0;

    virtual void Stop() = 0;

    // std::vector<WorkerPtr> ListWorkers() = 0;

    virtual std::vector<DBItem> ListUserDBs() = 0;

    // AddFeatures to this db, we need find proper shard to store these fts.
    virtual RetCode AddFeatures(const std::string& db_id, const std::vector<Feature>& fts) = 0;

    // SearchFeatures in this db.
    virtual std::vector<FeatureScore> SearchFeature(const std::string& db_id, const Feature& query,
                                                    int topk)
        = 0;
};

class Worker {
  public:
    virtual std::string GetWorkerID() = 0;

    virtual uint64 GetFreeSpace() = 0;

    // ListShards report all shards this worker is serving.
    virtual std::vector<DBShard> ListShards() = 0;

    // ServeShard let the worker serve this shard, for its features' CRUD
    virtual RetCode ServeShard(const DBShard& shard_info) = 0;

    // CloseShard close db_id/shard_id.
    virtual RetCode CloseShard(const std::string& db_id, const std::string& shard_id) = 0;

    // AddFeatures to db_id/shard_id, delegate to remote worker.
    virtual RetCode AddFeatures(const std::string& db_id, const std::string& shard_id,
                                const std::vector<Feature>& fts)
        = 0;

    // Search feature in the worker. worker can have multiple dbs, multiple shards.
    // only search in the requested db.
    virtual std::vector<FeatureScore> SearchFeature(const std::string& db_id, const Feature& query,
                                                    int topk)
        = 0;
};

using WorkerPtr = shared_ptr<Worker>;

class ShardManager;
class Shard {

  public:
    virtual ~Shard() = default;

    virtual void Start() = 0;
    virtual void Stop() = 0;

    // Assign a worker for this shard.
    virtual RetCode AssignWorker(Worker* worker) = 0;

    // AddFeatures to this shard, delegate to worker client to do the actual storage.
    virtual RetCode AddFeatures(const std::vector<Feature>& fts) = 0;

    // SearchFeature in this shard, delegate to worker client to do the actual search.
    virtual std::vector<FeatureScore> SearchFeature(const Feature& query, int topk) = 0;

    virtual RetCode Close() = 0;

    // check the shard has been assigned worker or not.
    virtual bool HasWorker() = 0;

    // check the shard is closed or not.
    virtual bool IsClosed() = 0;

    // check the shard is closed or not.
    virtual bool IsRunning() = 0;

    virtual std::string GetShardID() = 0;

    virtual DBShard GetShardInfo() = 0;
};

class ShardFactory {
  public:
    virtual ~ShardFactory() = default;
    virtual Shard* CreateShard(ShardManager* mgr, DBShard shard_info) = 0;
};

class ShardManager {

  public:
    virtual ~ShardManager() = default;

    virtual std::tuple<Shard*, bool> FindOrCreateWritableShard(std::string db_id, uint64 fts_count)
        = 0;

    virtual RetCode AssignWorkerToShard(Shard* shard, Worker* worker) = 0;

    virtual std::vector<DBItem> ListUserDBs() = 0;

    virtual std::vector<Shard*> ListShards(std::string db_id) = 0;

    virtual RetCode CloseShard(std::string db_id, std::string shard_id) = 0;

    virtual std::string CreateShard(DBShard shard_info) = 0;

    virtual RetCode UpdateShard(DBShard shard_info) = 0;
};

} // namespace search_manager
} // namespace feature_search
} // namespace donde
