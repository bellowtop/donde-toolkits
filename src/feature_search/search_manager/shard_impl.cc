#include "shard_impl.h"

#include "donde/definitions.h"
#include "shard_manager.h"

#include <exception>
#include <memory>
#include <mutex>
#include <spdlog/spdlog.h>

namespace donde_toolkits ::feature_search ::search_manager {

const int WAIT_MSG_INTERVAL_MS = 3000;

Shard* ShardFactoryImpl::CreateShard(ShardManager* mgr, DBShard shard_info) {
    return new ShardImpl(mgr, shard_info);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Shard
////////////////////////////////////////////////////////////////////////////////////////////////////////////

ShardImpl::ShardImpl(ShardManager* manager, DBShard shard_info)
    : _shard_info(shard_info),
      _shard_id(shard_info.shard_id),
      _db_id(shard_info.db_id),
      _shard_mgr(manager),
      _is_stopped(true),
      _is_closed(true) {
    Start();
    Open();
};

ShardImpl::~ShardImpl() { Stop(); };

void ShardImpl::Start() {
    std::lock_guard<std::mutex> l(_thread_mu);

    if (!_is_stopped.load()) {
        spdlog::warn("shard already is running, double Start??.");
        return;
    }

    // create new thread
    _loop_thread.reset(new std::thread(&ShardImpl::loop, std::ref(*this)));
    _channel.reset(new MsgChannel());

    _is_stopped.store(false);
};

void ShardImpl::Stop() {
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

RetCode ShardImpl::Open() {
    if (!_is_closed.load()) {
        spdlog::warn("shard already is opened, double Open??.");
        return RET_ERR;
    }
    _is_closed.store(false);
    return RetCode::RET_OK;
};

RetCode ShardImpl::Close() {
    if (_is_closed.load()) {
        spdlog::warn("shard already is closed, double Close??.");
        return RET_ERR;
    }
    _is_closed.store(true);
    return RetCode::RET_OK;
};

//////////////////////////////////////////////////////////////////////////////////
// Feature management
//////////////////////////////////////////////////////////////////////////////////

// Assign a worker for this shard.
RetCode ShardImpl::AssignWorker(Worker* worker) {
    if (_is_stopped.load() || _is_closed.load()) {
        spdlog::info("shard is already stopped or closed!");
        return RetCode::RET_ERR;
    }

    if (HasWorker()) {
        spdlog::error("shard already has worker, double AssignWorker.");
        return RetCode::RET_ERR;
    }
    if (worker == nullptr) {
        spdlog::error("assigned null worker?");
        return RetCode::RET_ERR;
    }

    _worker = worker;
    _worker_id = worker->GetWorkerID();

    return RetCode::RET_OK;
};

// AddFeatures to this shard, delegate to worker client to do the actual storage.
std::vector<std::string> ShardImpl::AddFeatures(const std::vector<Feature>& fts) {
    if (_is_stopped.load() || _is_closed.load()) {
        spdlog::info("shard is already stopped or closed!");
        return {};
    }

    if (!HasWorker()) {
        spdlog::error("shard has no worker, AssignWorker first.");
        return {};
    }

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
// Note if shard is closed, it still can be Searched.
std::vector<FeatureSearchItem> ShardImpl::SearchFeature(const Feature& query, int topk) {
    if (_is_stopped.load() /*|| _is_closed.load()*/) {
        spdlog::info("shard is already stopped!");
        return {};
    }

    if (!HasWorker()) {
        spdlog::error("shard has no worker, AssignWorker first.");
        return {};
    }

    auto msg = NewWorkMessage(shardOp{
        .valueType = searchFeatureReqType,
        .valuePtr = std::shared_ptr<searchFeatureReq>(new searchFeatureReq{query, topk}),
    });
    _channel->enqueueNotification(msg);
    auto output = msg->waitResponse();
    auto value = std::static_pointer_cast<searchFeatureRsp>(output.valuePtr);

    return std::move(value->fts);
};

void ShardImpl::loop() {
    for (;;) {

        // output is a blocking call.
        Notification::Ptr pNf = _channel->waitDequeueNotification(WAIT_MSG_INTERVAL_MS);
        if (pNf.isNull()) {
            spdlog::info("get null message, continue loop");
            // drain queue.
            if (_is_stopped.load()) {
                spdlog::info("get stopped flag, break loop");
                break;
            }
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
        default:
            spdlog::error("shard loop input value is not valid! wrong valueType: ");
            continue;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Inner implements.
////////////////////////////////////////////////////////////////////////////////////////////////////////////

shardOp ShardImpl::do_add_features(const shardOp& input) {
    auto req = std::static_pointer_cast<addFeaturesReq>(input.valuePtr);
    auto rsp = std::make_shared<addFeaturesRsp>();

    // delegate to worker.
    auto ret = _worker->AddFeatures(_db_id, _shard_id, req->fts);
    if (ret.size() > 0) {
        _shard_info.used += req->fts.size();
        _shard_mgr->UpdateShard(_shard_info);
        rsp->feature_ids = ret;
    }

    shardOp output{
        .valueType = addFeaturesRspType,
        .valuePtr = rsp,
    };
    return output;
};

shardOp ShardImpl::do_search_feature(const shardOp& input) {
    auto req = std::static_pointer_cast<searchFeatureReq>(input.valuePtr);
    auto rsp = std::make_shared<searchFeatureRsp>();

    // TODO: what about _worker api error?
    auto ret = _worker->SearchFeature(_shard_info.db_id, req->query, req->topk);
    rsp->fts.swap(ret);

    shardOp output{
        .valueType = searchFeatureRspType,
        .valuePtr = rsp,
    };
    return output;
};

} // namespace donde_toolkits::feature_search::search_manager
