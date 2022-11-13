#pragma once

#include "message.h"
#include "search/definitions.h"
#include "search/driver.h"
#include "search_manager/worker_api.h"
#include "types.h"
#include "utils.h"

#include <Poco/Thread.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

class ShardManager;

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

class Shard {

  public:
    Shard(ShardManager* manager, search::DBShard shard_info);
    ~Shard();

    void Start();
    void Stop();

    // Assign a worker for this shard.
    RetCode AssignWorker(Worker* worker);

    // AddFeatures to this shard, delegate to worker client to do the actual storage.
    RetCode AddFeatures(const std::vector<Feature>& fts);

    // SearchFeature in this shard, delegate to worker client to do the actual search.
    std::vector<FeatureScore> SearchFeature(const Feature& query, int topk);

    RetCode Close();

    // check the shard has been assigned worker or not.
    inline bool HasWorker() { return _worker == nullptr; };

    // check the shard is closed or not.
    inline bool IsClosed() { return _shard_info.is_closed; };

    // check the shard is closed or not.
    inline bool IsRunning() { return _loop_thread.isRunning(); };

    inline std::string GetShardID() { return _shard_id; };

    inline search::DBShard GetShardInfo() { return _shard_info; };

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
