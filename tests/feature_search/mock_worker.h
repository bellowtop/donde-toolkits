#include "src/feature_search/search_manager/worker.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace donde_toolkits;
using namespace donde_toolkits::feature_search::search_manager;
using namespace donde_toolkits::feature_search;

class MockWorker : public Worker {

  public:
    MOCK_METHOD(std::string, GetWorkerID, ());

    MOCK_METHOD(uint64, GetFreeSpace, ());

    // ListShards report all shards this worker is serving.
    MOCK_METHOD(std::vector<DBShard>, ListShards, ());

    // ServeShard let the worker serve this shard, for its features' CRUD
    MOCK_METHOD(RetCode, ServeShard, (const DBShard& shard_info));

    // CloseShard close db_id/shard_id.
    MOCK_METHOD(RetCode, CloseShard, (const std::string& db_id, const std::string& shard_id));

    // AddFeatures to db_id/shard_id, delegate to remote worker.
    MOCK_METHOD(std::vector<std::string>, AddFeatures,
                (const std::string& db_id, const std::string& shard_id,
                 const std::vector<Feature>& fts));

    // Search feature in the worker. worker can have multiple dbs, multiple shards.
    // only search in the requested db.
    MOCK_METHOD(std::vector<FeatureSearchItem>, SearchFeature,
                (const std::string& db_id, const Feature& query, int topk));
};
