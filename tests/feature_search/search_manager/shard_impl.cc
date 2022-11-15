#include "source/feature_search/search_manager/shard_impl.h"

#include "donde/feature_search/api.h"
#include "donde/feature_search/definitions.h"
#include "donde/feature_search/search_manager/shard_manager.h"
#include "donde/utils.h"
#include "tests/feature_search/mock_shard_manager.h"
#include "tests/feature_search/mock_worker.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
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

    EXPECT_EQ(impl.IsRunning(), true);

    impl.Stop();

    // let the shard loop run...
    std::this_thread::sleep_for(std::chrono::seconds(1));

    EXPECT_EQ(impl.IsRunning(), false);

    // manually start again
    impl.Start();

    EXPECT_EQ(impl.IsRunning(), true);

    impl.Stop();
    // let the shard loop run...
    std::this_thread::sleep_for(std::chrono::seconds(1));

    EXPECT_EQ(impl.IsRunning(), false);
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
}

} // namespace
