#include "mem_shard_impl.h"

#include "donde/definitions.h"
#include "donde/feature_search/definitions.h"
#include "source/feature_search/feature_topk_rank.h"

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

MemoryShardImpl::MemoryShardImpl(ShardManager& manager, Driver& driver, DBShard shard_info)
    : Shard(manager, shard_info),
      _shard_info(shard_info),
      _shard_id(shard_info.shard_id),
      _db_id(shard_info.db_id),
      _shard_mgr(manager),
      _driver(driver),
      _is_loaded(false),
      _is_stopped(true) {

    Start();
    Load();
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

// Load features into this shard.
void MemoryShardImpl::Load() {
    auto input = shardOp{
        .valueType = loadFeaturesReqType,
        .valuePtr = std::shared_ptr<loadFeaturesReq>(new loadFeaturesReq{}),
    };

    auto msg = NewWorkMessage(input);
    _channel->enqueueNotification(msg);
    auto output = msg->waitResponse();
    msg->giveReceipt();
    auto value = std::static_pointer_cast<loadFeaturesRsp>(output.valuePtr);
    return;
};

//////////////////////////////////////////////////////////////////////////////////
// Feature management
//////////////////////////////////////////////////////////////////////////////////

// AddFeatures to this shard, delegate to worker client to do the actual storage.
std::vector<std::string> MemoryShardImpl::AddFeatures(const std::vector<FeatureDbItem>& fts) {
    auto input = shardOp{
        .valueType = addFeaturesReqType,
        .valuePtr = std::shared_ptr<addFeaturesReq>(new addFeaturesReq{fts}),
    };

    auto msg = NewWorkMessage(input);
    _channel->enqueueNotification(msg);
    auto output = msg->waitResponse();
    msg->giveReceipt();
    auto value = std::static_pointer_cast<addFeaturesRsp>(output.valuePtr);
    return value->feature_ids;
};

// RemoveFeatures from this shard
RetCode MemoryShardImpl::RemoveFeatures(const std::vector<std::string>& feature_ids) {
    auto input = shardOp{
        .valueType = removeFeaturesReqType,
        .valuePtr = std::shared_ptr<removeFeaturesReq>(new removeFeaturesReq{feature_ids}),
    };

    auto msg = NewWorkMessage(input);
    _channel->enqueueNotification(msg);
    auto output = msg->waitResponse();
    msg->giveReceipt();
    auto value = std::static_pointer_cast<removeFeaturesRsp>(output.valuePtr);

    return RetCode::RET_OK;
};

// SearchFeature in this shard, delegate to worker client to do the actual search.
std::vector<FeatureSearchItem> MemoryShardImpl::SearchFeature(const Feature& query, int topk) {
    auto msg = NewWorkMessage(shardOp{
        .valueType = searchFeatureReqType,
        .valuePtr = std::shared_ptr<searchFeatureReq>(new searchFeatureReq{query, topk}),
    });
    _channel->enqueueNotification(msg);
    auto output = msg->waitResponse();
    msg->giveReceipt();
    auto value = std::static_pointer_cast<searchFeatureRsp>(output.valuePtr);

    return std::move(value->fts);
};

RetCode MemoryShardImpl::Close() {
    auto msg = NewWorkMessage(shardOp{closeShardReqType});
    _channel->enqueueNotification(msg);
    auto output = msg->waitResponse();
    msg->giveReceipt();
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

        // NOTICE: if pNF go out of scope, the `msg` will be freed!
        // so if you need to use it, use it early!
        WorkMessage<shardOp>::Ptr msg = pNf.cast<WorkMessage<shardOp>>();
        auto input = msg->getRequest();

        switch (input.valueType) {

        case loadFeaturesReqType: {
            auto output = do_load_features(input);
            // set a response and wait for a receipt, blocking call.
            // blocking for at most 1s (default).
            msg->setResponse(output, true);
            break;
        }
        case addFeaturesReqType: {
            auto output = do_add_features(input);
            msg->setResponse(output, true);
            break;
        }
        case removeFeaturesReqType: {
            auto output = do_remove_features(input);
            msg->setResponse(output, true);
            break;
        }
        case searchFeatureReqType: {
            auto output = do_search_feature(input);
            msg->setResponse(output, true);
            break;
        }
        case closeShardReqType: {
            auto output = do_close_shard(input);
            msg->setResponse(output, true);
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

shardOp MemoryShardImpl::do_load_features(const shardOp& input) {
    auto req = std::static_pointer_cast<loadFeaturesReq>(input.valuePtr);
    auto rsp = std::make_shared<loadFeaturesRsp>();

    // TODO

    _is_loaded.store(true);

    shardOp output{
        .valueType = loadFeaturesRspType,
        .valuePtr = rsp,
    };
    return output;
};

shardOp MemoryShardImpl::do_add_features(const shardOp& input) {
    auto req = std::static_pointer_cast<addFeaturesReq>(input.valuePtr);
    auto rsp = std::make_shared<addFeaturesRsp>();

    // TODO

    shardOp output{
        .valueType = addFeaturesRspType,
        .valuePtr = rsp,
    };
    return output;
};

shardOp MemoryShardImpl::do_remove_features(const shardOp& input) {
    auto req = std::static_pointer_cast<removeFeaturesReq>(input.valuePtr);
    auto rsp = std::make_shared<removeFeaturesRsp>();

    // TODO

    shardOp output{
        .valueType = removeFeaturesRspType,
        .valuePtr = rsp,
    };
    return output;
};

// brute force search each feature in db.
shardOp MemoryShardImpl::do_search_feature(const shardOp& input) {
    auto req = std::static_pointer_cast<searchFeatureReq>(input.valuePtr);
    auto rsp = std::make_shared<searchFeatureRsp>();

    Feature& query = req->query;
    int topk = req->topk;
    const std::string& db_id = _shard_info.db_id;
    const std::string& shard_id = _shard_info.shard_id;

    FeatureTopkRanking rank(query, topk);

    uint page = 0;
    uint perPage = 10;
    PageData<FeatureDbItemList> pageData = _driver.ListFeatures(page, perPage, db_id, shard_id);

    while (pageData.data.size() != 0) {
        std::vector<std::string> feature_ids = convert_to_feature_ids(pageData.data);
        std::vector<Feature> fts = _driver.LoadFeatures(feature_ids, db_id, shard_id);

        rank.FeedIn(fts);

        // read next page, caution: copy assignment here!
        pageData = _driver.ListFeatures(++page, perPage, db_id, shard_id);
    }

    std::vector<FeatureSearchItem> sorted = rank.SortOut();

    rsp->fts.swap(sorted);

    shardOp output{
        .valueType = searchFeatureRspType,
        .valuePtr = rsp,
    };
    return output;
};

shardOp MemoryShardImpl::do_close_shard(const shardOp& input) {
    auto req = std::static_pointer_cast<closeShardReq>(input.valuePtr);
    auto rsp = std::make_shared<closeShardRsp>();

    // TODO

    shardOp output{
        .valueType = closeShardRspType,
        .valuePtr = rsp,
    };
    return output;
};

} // namespace search_worker
} // namespace feature_search
} // namespace donde
