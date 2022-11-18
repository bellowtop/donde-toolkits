#pragma once

#include "donde/feature_search/api.h"
#include "donde/feature_search/search_worker/api.h"
#include "donde/message.h"
#include "donde/utils.h"

#include <Poco/Thread.h>
#include <memory>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std;

namespace donde {
namespace feature_search {
namespace search_worker {

class ShardManagerImpl;

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
    std::vector<FeatureScore> fts;
};

enum shardOpType {
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

class MemoryShardImpl : public Shard {

  public:
    MemoryShardImpl(ShardManager& shard_manager, DBShard shard_info);
    ~MemoryShardImpl();

    // Load features from driver; load index if needed.
    void Load();

    // Start the loop, for add/search etc.
    void Start();
    // Stop the loop.
    void Stop();

    // AddFeatures to this shard
    std::vector<std::string> AddFeatures(const std::vector<Feature>& fts);

    // SearchFeature in this shard, delegate to worker client to do the actual search.
    std::vector<FeatureScore> SearchFeature(const Feature& query, int topk);

    // Close this shard, cannot add features from this shard, but still can search.
    RetCode Close();

    // IsClosed return true if shard is closed.
    bool IsClosed() { return _shard_info.is_closed; };

    // IsStopped return true if shard loop is stopped. a stopped shard means it will not respond to
    // any api calls (add/search etc.)
    bool IsStopped() { return _is_stopped.load(); };

    // Quick methods
    std::string GetShardID() { return _shard_id; };
    DBShard GetShardInfo() { return _shard_info; };

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

    ShardManager& _shard_mgr;

    std::atomic<bool> _is_stopped;
    std::shared_ptr<MsgChannel> _channel;
    std::shared_ptr<std::thread> _loop_thread;
    std::mutex _thread_mu;
};

} // namespace search_worker
} // namespace feature_search
} // namespace donde
