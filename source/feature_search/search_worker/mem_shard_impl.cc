#include "mem_shard_impl.h"

#include "donde/definitions.h"

#include <exception>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>

namespace donde {
namespace feature_search {
namespace search_worker {

const int WAIT_MSG_INTERVAL_MS = 3000;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Shard
////////////////////////////////////////////////////////////////////////////////////////////////////////////

MemoryShardImpl::MemoryShardImpl(ShardManager& manager, DBShard shard_info)
    : Shard(manager, shard_info),
      _shard_info(shard_info),
      _shard_id(shard_info.shard_id),
      _db_id(shard_info.db_id),
      _shard_mgr(manager),
      _is_stopped(true) {

    Start();
};

MemoryShardImpl::~MemoryShardImpl() { Stop(); };

void MemoryShardImpl::Start() {
    std::lock_guard<std::mutex> l(_thread_mu);

    if (!_is_stopped.load()) {
        spdlog::warn("shard already is running, double Start??.");
        return;
    }

    // create new thread
    _loop_thread.reset(new std::thread(&MemoryShardImpl::loop, std::ref(*this)));
    _channel.reset(new MsgChannel());

    _is_stopped.store(false);
};

void MemoryShardImpl::Stop() {
    std::lock_guard<std::mutex> l(_thread_mu);

    if (_is_stopped.load()) {
        spdlog::warn("shard already is stopped, double Stop??.");
        return;
    }

    _is_stopped.store(true);
    _channel->wakeUpAll();

    try {
        if (_loop_thread) {
            _loop_thread->join();
            // release thread.
            _loop_thread.reset();
        }
    } catch (const std::exception& exc) {
        spdlog::warn("catch exc when join thread {}.", exc.what());
    }
    // release channel
    _channel.reset();

    std::cout << "after join" << std::endl;
};

void MemoryShardImpl::Load(){

};

//////////////////////////////////////////////////////////////////////////////////
// Feature management
//////////////////////////////////////////////////////////////////////////////////

// AddFeatures to this shard, delegate to worker client to do the actual storage.
std::vector<std::string> MemoryShardImpl::AddFeatures(const std::vector<Feature>& fts) {
    auto input = shardOp{
        .valueType = addFeaturesReqType,
        .valuePtr = std::shared_ptr<addFeaturesReq>(new addFeaturesReq{fts}),
    };

    auto msg = NewWorkMessage(input);
    _channel->enqueueNotification(msg);
    auto output = msg->waitResponse();
    auto value = std::static_pointer_cast<addFeaturesRsp>(output.valuePtr);
    return value->feature_ids;
};

// SearchFeature in this shard, delegate to worker client to do the actual search.
std::vector<FeatureScore> MemoryShardImpl::SearchFeature(const Feature& query, int topk) {
    auto msg = NewWorkMessage(shardOp{
        .valueType = searchFeatureReqType,
        .valuePtr = std::shared_ptr<searchFeatureReq>(new searchFeatureReq{query, topk}),
    });
    _channel->enqueueNotification(msg);
    auto output = msg->waitResponse();
    auto value = std::static_pointer_cast<searchFeatureRsp>(output.valuePtr);

    return std::move(value->fts);
};

RetCode MemoryShardImpl::Close() {
    auto msg = NewWorkMessage(shardOp{closeShardReqType});
    _channel->enqueueNotification(msg);
    auto output = msg->waitResponse();
    auto value = std::static_pointer_cast<closeShardRsp>(output.valuePtr);

    _shard_info.is_closed = true;

    Stop();

    return RetCode::RET_OK;
};

void MemoryShardImpl::loop() {
    for (;;) {
        if (_is_stopped.load()) {
            spdlog::info("get stopped flag, break loop");
            break;
        }
        // output is a blocking call.
        Notification::Ptr pNf = _channel->waitDequeueNotification(WAIT_MSG_INTERVAL_MS);
        if (pNf.isNull()) {
            spdlog::info("get null message, continue loop");
            continue;
        }

        WorkMessage<shardOp>::Ptr msg = pNf.cast<WorkMessage<shardOp>>();
        auto input = msg->getRequest();

        switch (input.valueType) {

        case addFeaturesReqType: {
            auto output = do_add_features(input);
            msg->setResponse(output);
            break;
        }
        case searchFeatureReqType: {
            auto output = do_search_feature(input);
            msg->setResponse(output);
            break;
        }
        case closeShardReqType: {
            auto output = do_close_shard(input);
            msg->setResponse(output);
            break;
        }
        default:
            spdlog::error("shard loop input value is not valid! wrong valueType: ");
            continue;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Inner implements.
////////////////////////////////////////////////////////////////////////////////////////////////////////////

shardOp MemoryShardImpl::do_add_features(const shardOp& input) {
    auto req = std::static_pointer_cast<addFeaturesReq>(input.valuePtr);
    auto rsp = std::make_shared<addFeaturesRsp>();

    shardOp output{
        .valueType = addFeaturesRspType,
        .valuePtr = rsp,
    };
    return output;
};
shardOp MemoryShardImpl::do_search_feature(const shardOp& input) {
    auto req = std::static_pointer_cast<searchFeatureReq>(input.valuePtr);
    auto rsp = std::make_shared<searchFeatureRsp>();

    shardOp output{
        .valueType = searchFeatureRspType,
        .valuePtr = rsp,
    };
    return output;
};

shardOp MemoryShardImpl::do_close_shard(const shardOp& input) {
    auto req = std::static_pointer_cast<closeShardReq>(input.valuePtr);
    auto rsp = std::make_shared<closeShardRsp>();

    shardOp output{
        .valueType = closeShardRspType,
        .valuePtr = rsp,
    };
    return output;
};

} // namespace search_worker
} // namespace feature_search
} // namespace donde
