#pragma once

#include "definitions.h"
#include "message.h"
#include "search/api.h"
#include "search_manager/api.h"
#include "utils.h"

#include <Poco/Thread.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

class ShardManagerImpl;

struct assignWorkerReq {};
struct assignWorkerRsp {};

struct addFeaturesReq {
    std::vector<Feature> fts;
};
struct addFeaturesRsp {};

struct closeShardReq {};
struct closeShardRsp {};

struct searchFeatureReq {
    Feature query;
    int topk;
};
struct searchFeatureRsp {
    std::vector<FeatureScore> fts;
};

enum shardOpType {
    assignWorkerReqType,
    assignWorkerRspType,

    addFeaturesReqType,
    addFeaturesRspType,

    closeShardReqType,
    closeShardRspType,

    searchFeatureRspType,
    searchFeatureReqType,

};

struct shardOp {
    shardOpType valueType;
    std::shared_ptr<void> valuePtr;
};

class ShardImpl : public Shard {

  public:
    ShardImpl(ShardManager* manager, search::DBShard shard_info);
    ~ShardImpl();

    void Start() override;
    void Stop() override;

    // Assign a worker for this shard.
    RetCode AssignWorker(Worker* worker) override;

    // AddFeatures to this shard, delegate to worker client to do the actual storage.
    RetCode AddFeatures(const std::vector<Feature>& fts) override;

    // SearchFeature in this shard, delegate to worker client to do the actual search.
    std::vector<FeatureScore> SearchFeature(const Feature& query, int topk) override;

    RetCode Close() override;

    // check the shard has been assigned worker or not.
    inline bool HasWorker() override { return _worker == nullptr; };

    // check the shard is closed or not.
    inline bool IsClosed() override { return _shard_info.is_closed; };

    // check the shard is closed or not.
    inline bool IsRunning() override { return _loop_thread.isRunning(); };

    inline std::string GetShardID() override { return _shard_id; };

    inline search::DBShard GetShardInfo() override { return _shard_info; };

  private:
    void loop();

    shardOp do_assign_worker(const shardOp& input);
    shardOp do_add_features(const shardOp& input);
    shardOp do_search_feature(const shardOp& input);
    shardOp do_close_shard(const shardOp& input);

  private:
    search::DBShard _shard_info;

    std::string _shard_id;
    std::string _db_id;

    std::string _worker_id;
    Worker* _worker = nullptr;

    ShardManager* _shard_mgr = nullptr;

    std::shared_ptr<MsgChannel> _channel;
    Poco::Thread _loop_thread;
};

class ShardFactoryImpl : public ShardFactory {
  public:
    Shard* CreateShard(ShardManager* mgr, search::DBShard shard_info) override;
};
