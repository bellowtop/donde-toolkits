#include "source/feature_search/search_manager/shard_impl.h"

#include "donde/definitions.h"
#include "donde/feature_search/api.h"
#include "donde/feature_search/definitions.h"
#include "donde/feature_search/search_manager/shard_manager.h"
#include "donde/utils.h"
#include "tests/feature_search/mock_shard_manager.h"
#include "tests/feature_search/mock_worker.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <gmock/gmock-nice-strict.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <ostream>
#include <thread>

using namespace std;

using nlohmann::json;

using namespace donde::feature_search;
using namespace donde::feature_search::search_manager;

using testing::NiceMock;

namespace {

class SearchManager_Shard : public ::testing::Test {
  protected:
    void SetUp() override{

    };

    void TearDown() override{};
};

TEST_F(SearchManager_Shard, CanStartStop) {
    DBShard shard_info;

    MockShardManager mMgr;
    ShardImpl impl(&mMgr, shard_info);

    // already start by constructor.
    // impl.Start();

    EXPECT_EQ(impl.IsStopped(), false);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    impl.Stop();
    EXPECT_EQ(impl.IsStopped(), true);

    // manually start again
    impl.Start();
    EXPECT_EQ(impl.IsStopped(), false);
    // let it running for a while
    std::this_thread::sleep_for(std::chrono::seconds(1));

    impl.Stop();
    EXPECT_EQ(impl.IsStopped(), true);
};

TEST_F(SearchManager_Shard, CanAssignWorker) {
    DBShard shard_info;

    MockShardManager mMgr;
    ShardImpl impl(&mMgr, shard_info);

    EXPECT_EQ(impl.HasWorker(), false);

    MockWorker mWorker;
    EXPECT_CALL(mWorker, GetWorkerID);
    EXPECT_CALL(mWorker, ServeShard);

    impl.AssignWorker(&mWorker);

    // let the loop run...
    std::this_thread::sleep_for(std::chrono::seconds(1));

    EXPECT_EQ(impl.HasWorker(), true);
};

TEST_F(SearchManager_Shard, CanCloseShard) {
    DBShard shard_info;

    MockShardManager mMgr;
    ShardImpl impl(&mMgr, shard_info);

    // 1. no worker yet
    EXPECT_EQ(impl.HasWorker(), false);

    EXPECT_EQ(impl.Close(), RetCode::RET_ERR);

    // 2. after assign worker
    NiceMock<MockWorker> mWorker;
    EXPECT_CALL(mWorker, CloseShard);
    EXPECT_CALL(mMgr, CloseShard);

    impl.AssignWorker(&mWorker);

    EXPECT_EQ(impl.HasWorker(), true);

    // 3. now can close, and worker.CloseShard must be called.
    EXPECT_EQ(impl.Close(), RetCode::RET_OK);

    // let the loop run...
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

} // namespace
