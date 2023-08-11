#pragma once

#include "definitions.h"
// #include "faiss/Index2Layer.h"
#include "nlohmann/json.hpp"

#include <map>
#include <memory>
#include <opencv2/core/hal/interface.h>

using namespace std;

using json = nlohmann::json;

namespace donde_toolkits {

namespace feature_search {

// Driver contains all databases infomation for feature search.
// User features are stored in UserDBs
// Each UserDB may have several Shards, sharding based on shard size & shard time period
// Each Shard is bound with one Worker, only when the worker comes online, the shard then can be
// accessed(eg read/write)
//
// Note that Shards are logic management unit for feature operations,
// the underlying features are still stored in one big table.
//
// tables:
//    user_dbs(db_id, name, size, description, created_at, user_identifier)
//    shards(shard_id, db_id, worker_id, created_at, size, used, free)
//    workers(worker_id, address, max_capacity)
//    features(feature_id, feature, shard_id)

class Driver {

  public:
    virtual ~Driver() = default;

    virtual RetCode Init(const std::vector<std::string>& initial_db_ids) = 0;

    // DB management
    virtual std::string CreateDB(const DBItem& info) = 0;

    virtual DBItem FindDB(const std::string& db_id) = 0;

    virtual std::vector<DBItem> ListDBs() = 0;

    virtual RetCode DeleteDB(std::string db_id) = 0;

    // Worker Management
    virtual std::vector<WorkerItem> ListWorkers() = 0;

    virtual void CreateWorker(const std::string& worker_id, const WorkerItem& worker) = 0;

    virtual void UpdateWorker(const std::string& worker_id, const WorkerItem& worker) = 0;

    // Shard Management
    virtual std::vector<DBShard> ListShards(const std::string& db_id) = 0;

    virtual std::string CreateShard(const std::string& db_id, const DBShard& shard) = 0;

    virtual RetCode UpdateShard(const std::string& db_id, const DBShard& shard) = 0;

    virtual std::string CloseShard(const std::string& db_id, const std::string& shard_id) = 0;

    // Feature management
    virtual PageData<FeatureDbItemList> ListFeatures(uint page, uint perPage,
                                                     const std::string& db_id,
                                                     const std::string& shard_id = "")
        = 0;

    virtual std::vector<std::string> AddFeatures(const std::vector<FeatureDbItem>& features,
                                                 const std::string& db_id,
                                                 const std::string& shard_id)
        = 0;

    virtual std::vector<Feature> LoadFeatures(const std::vector<std::string>& feature_ids,
                                              const std::string& db_id, const std::string& shard_id)
        = 0;

    virtual RetCode RemoveFeatures(const std::vector<std::string>& feature_ids,
                                   const std::string& db_id, const std::string& shard_id = "")
        = 0;
};

} // namespace feature_search

} // namespace donde_toolkits
