#include "src/feature_search/search_manager/shard_impl.h"

#include "../mock_shard_manager.h"
#include "../mock_worker.h"
#include "donde/definitions.h"
#include "donde/feature_search/definitions.h"
#include "donde/utils.h"
#include "src/feature_search/search_manager/shard_manager.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <gmock/gmock-actions.h>
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

using namespace donde_toolkits::feature_search;
using namespace donde_toolkits::feature_search::search_manager;

using testing::NiceMock;

namespace search_manager {

class TestShardImpl : public ::testing::Test {
  protected:
    void SetUp() override{

    };

    std::vector<FeatureDbItem> generate_features(int feature_count) {
        std::vector<FeatureDbItem> ret;
        for (int i = 0; i < feature_count; i++) {
            auto ft = gen_feature_dim<512>();
            std::map<string, string> meta{{"keya", "valueb"}};
            ret.push_back(FeatureDbItem{
                .feature = ft,
                .metadata = meta,
            });
        }
        return ret;
    };

    void TearDown() override{};
};

TEST_F(TestShardImpl, CanGetInfo) {
    DBShard shard_info{
        .db_id = "foo",
        .shard_id = "bar",
        .capacity = 1000,
        .used = 100,
        .is_closed = false,
    };

    MockShardManager mMgr;
    ShardImpl impl(&mMgr, shard_info);

    auto got = impl.GetShardInfo();

    EXPECT_EQ(got.db_id, shard_info.db_id);
    EXPECT_EQ(got.shard_id, shard_info.shard_id);
    EXPECT_EQ(got.capacity, shard_info.capacity);
    EXPECT_EQ(got.used, shard_info.used);
    EXPECT_EQ(got.is_closed, shard_info.is_closed);

    // shortcut
    EXPECT_EQ(impl.GetShardID(), shard_info.shard_id);
    EXPECT_EQ(impl.IsClosed(), shard_info.is_closed);
};

TEST_F(TestShardImpl, CanStartStop) {
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

TEST_F(TestShardImpl, CanCloseShard) {
    DBShard shard_info;

    MockShardManager mMgr;
    ShardImpl impl(&mMgr, shard_info);

    // 1. no worker yet, so api return err.
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
TEST_F(TestShardImpl, CanAssignWorker) {
    DBShard shard_info;

    MockShardManager mMgr;
    ShardImpl impl(&mMgr, shard_info);

    // 1. no worker yet, so api return err.
    EXPECT_EQ(impl.HasWorker(), false);
    EXPECT_EQ(impl.Close(), RetCode::RET_ERR);

    MockWorker mWorker;
    EXPECT_CALL(mWorker, GetWorkerID);
    EXPECT_CALL(mWorker, ServeShard);

    impl.AssignWorker(&mWorker);

    // let the loop run...
    std::this_thread::sleep_for(std::chrono::seconds(1));

    EXPECT_EQ(impl.HasWorker(), true);
};

TEST_F(TestShardImpl, CanAddFeatures) {
    DBShard shard_info;

    MockShardManager mMgr;
    ShardImpl impl(&mMgr, shard_info);

    // 1. no worker yet, so api return err.
    EXPECT_EQ(impl.HasWorker(), false);
    EXPECT_EQ(impl.Close(), RetCode::RET_ERR);

    NiceMock<MockWorker> mWorker;
    impl.AssignWorker(&mWorker);

    auto used1 = impl.GetShardInfo().used;

    // set expect.
    std::vector<std::string> ft_ids(10);
    EXPECT_CALL(mWorker, AddFeatures).WillOnce(testing::Return(ft_ids));
    EXPECT_CALL(mMgr, UpdateShard).WillOnce(testing::Return(RetCode::RET_OK));

    // do api
    std::vector<Feature> fts(10);
    impl.AddFeatures(fts);

    // let the loop run...
    std::this_thread::sleep_for(std::chrono::seconds(1));

    auto used2 = impl.GetShardInfo().used;
    EXPECT_EQ(used1 + 10, used2);
};

TEST_F(TestShardImpl, CanSearchFeature) {
    DBShard shard_info;

    MockShardManager mMgr;
    ShardImpl impl(&mMgr, shard_info);

    // 1. no worker yet, so api return err.
    EXPECT_EQ(impl.HasWorker(), false);
    EXPECT_EQ(impl.Close(), RetCode::RET_ERR);

    NiceMock<MockWorker> mWorker;
    impl.AssignWorker(&mWorker);

    auto used1 = impl.GetShardInfo().used;

    // set expect.
    std::vector<std::string> ft_ids(10);
    EXPECT_CALL(mWorker, AddFeatures).WillOnce(testing::Return(ft_ids));
    EXPECT_CALL(mMgr, UpdateShard).WillOnce(testing::Return(RetCode::RET_OK));

    // do api
    std::vector<Feature> fts(10);
    impl.AddFeatures(fts);

    // let the loop run...
    std::this_thread::sleep_for(std::chrono::seconds(1));

    auto used2 = impl.GetShardInfo().used;
    EXPECT_EQ(used1 + 10, used2);
};

} // namespace search_manager
