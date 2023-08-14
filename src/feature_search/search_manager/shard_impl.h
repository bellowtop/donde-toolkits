#pragma once

#include "donde/feature_search/definitions.h"
#include "donde/feature_search/shard.h"
#include "donde/feature_search/worker.h"
#include "donde/message.h"
#include "shard_factory.h"
#include "shard_manager.h"

#include <Poco/Thread.h>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace donde_toolkits ::feature_search ::search_manager {

class ShardManagerImpl;

struct assignWorkerReq {};
struct assignWorkerRsp {};

struct addFeaturesReq {
    std::vector<Feature> fts;
};
struct addFeaturesRsp {
    std::vector<std::string> feature_ids;
};

struct closeShardReq {};
struct closeShardRsp {};

struct searchFeatureReq {
    Feature query;
    int topk;
};
struct searchFeatureRsp {
    std::vector<FeatureSearchItem> fts;
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
    ShardImpl(ShardManager* manager, DBShard shard_info);
    ~ShardImpl();

    void Start() override;
    void Stop() override;

    // Assign a worker for this shard.
    RetCode AssignWorker(Worker* worker) override;

    // AddFeatures to this shard, delegate to worker client to do the actual storage.
    std::vector<std::string> AddFeatures(const std::vector<Feature>& fts) override;

    // SearchFeature in this shard, delegate to worker client to do the actual search.
    std::vector<FeatureSearchItem> SearchFeature(const Feature& query, int topk) override;

    RetCode Close() override;

    // check the shard has been assigned worker or not.
    inline bool HasWorker() override { return _worker != nullptr; };

    // check the shard is closed or not.
    inline bool IsClosed() override { return _shard_info.is_closed; };

    // check the shard is closed or not.
    inline bool IsStopped() override { return _is_stopped.load(); };

    inline std::string GetShardID() override { return _shard_id; };

    inline DBShard GetShardInfo() override { return _shard_info; };

  private:
    void loop();

    shardOp do_assign_worker(const shardOp& input);
    shardOp do_add_features(const shardOp& input);
    shardOp do_search_feature(const shardOp& input);
    shardOp do_close_shard(const shardOp& input);

  private:
    DBShard _shard_info;

    std::string _shard_id;
    std::string _db_id;

    std::string _worker_id;
    Worker* _worker = nullptr;

    ShardManager* _shard_mgr = nullptr;

    std::atomic<bool> _is_stopped;
    std::shared_ptr<MsgChannel> _channel;
    std::shared_ptr<std::thread> _loop_thread;
    std::mutex _thread_mu;
};

class ShardFactoryImpl : public ShardFactory {
  public:
    Shard* CreateShard(ShardManager* mgr, DBShard shard_info) override;
};

} // namespace donde_toolkits::feature_search::search_manager
