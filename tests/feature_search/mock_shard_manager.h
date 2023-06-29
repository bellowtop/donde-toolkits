#include "donde/feature_search/api.h"
#include "donde/feature_search/definitions.h"
#include "donde/feature_search/search_manager/shard_manager.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace donde_toolkits;
using namespace donde_toolkits::feature_search::search_manager;
using namespace donde_toolkits::feature_search;

class MockShardManager : public ShardManager {

  public:
    MOCK_METHOD((std::tuple<Shard*, bool>), FindOrCreateWritableShard,
                (std::string db_id, uint64 fts_count));

    MOCK_METHOD(RetCode, AssignWorkerToShard, (Shard * shard, Worker* worker));

    MOCK_METHOD(std::vector<DBItem>, ListUserDBs, ());

    MOCK_METHOD(std::vector<Shard*>, ListShards, (std::string db_id));

    MOCK_METHOD(RetCode, CloseShard, (std::string db_id, std::string shard_id));

    MOCK_METHOD(std::string, CreateShard, (DBShard shard_info));

    MOCK_METHOD(RetCode, UpdateShard, (DBShard shard_info));
};
