#include "search_manager/shard.h"

#include "message.h"
#include "search/definitions.h"
#include "search_manager/shard_manager.h"
#include "types.h"
#include "utils.h"

#include <Poco/RunnableAdapter.h>
#include <memory>
#include <spdlog/spdlog.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Shard
////////////////////////////////////////////////////////////////////////////////////////////////////////////

Shard::Shard(ShardManager* manager, search::DBShard shard_info)
    : _shard_info(shard_info),
      _shard_id(shard_info.shard_id),
      _db_id(shard_info.db_id),
      _channel(new MsgChannel()) {

    Poco::RunnableAdapter<Shard> ra(*this, &Shard::loop);
    _loop_thread.start(ra);
};

Shard::~Shard() { stop(); };

// Assign a worker for this shard.
RetCode Shard::AssignWorker(Worker* worker) {
    if (_worker != nullptr) {
        spdlog::error("shard already has worker, double AssignWorker??.");
        return RetCode::RET_ERR;
    }
    if (worker != nullptr) {
        spdlog::error("assigned null worker?");
        return RetCode::RET_ERR;
    }

    _worker = worker;
    _worker_id = worker->GetWorkerID();

    auto msg = NewWorkMessage(shardOp{assignWorkerReqType});
    _channel->input(msg);
    auto output = msg->waitResponse();
    auto value = std::static_pointer_cast<assignWorkerRsp>(output.valuePtr);

    return RetCode::RET_OK;
};

// AddFeatures to this shard, delegate to worker client to do the actual storage.
RetCode Shard::AddFeatures(const std::vector<Feature>& fts) {
    if (_worker == nullptr) {
        spdlog::error("shard has no worker, AssignWorker first.");
        return RetCode::RET_ERR;
    }

    ;
    auto input = shardOp{
        .valueType = addFeaturesReqType,
        .valuePtr = std::shared_ptr<addFeaturesReq>(new addFeaturesReq{fts}),
    };

    auto msg = NewWorkMessage(input);
    _channel->input(msg);
    auto output = msg->waitResponse();
    auto value = std::static_pointer_cast<addFeaturesRsp>(output.valuePtr);

    return RetCode::RET_OK;
};

// SearchFeature in this shard, delegate to worker client to do the actual search.
std::vector<FeatureScore> Shard::SearchFeature(const Feature& query, int topk) {
    if (_worker == nullptr) {
        spdlog::error("shard has no worker, AssignWorker first.");
        return {};
    }

    auto msg = NewWorkMessage(shardOp{
        .valueType = searchFeatureReqType,
        .valuePtr = std::shared_ptr<searchFeatureReq>(new searchFeatureReq{query, topk}),
    });
    _channel->input(msg);
    auto output = msg->waitResponse();
    auto value = std::static_pointer_cast<searchFeatureRsp>(output.valuePtr);

    return std::move(value->fts);
};

RetCode Shard::Close() {
    auto msg = NewWorkMessage(shardOp{closeShardReqType});
    _channel->input(msg);
    auto output = msg->waitResponse();
    auto value = std::static_pointer_cast<closeShardRsp>(output.valuePtr);

    _shard_info.is_closed = true;

    stop();

    return RetCode::RET_OK;
};

void Shard::loop() {
    for (;;) {
        Notification::Ptr pNf;
        // output is a blocking api.
        if (_channel->output(pNf) == ChanError::ErrClosed) {
            break;
        }

        if (!pNf.isNull()) {
            WorkMessage<shardOp>::Ptr msg = pNf.cast<WorkMessage<shardOp>>();
            auto input = msg->getRequest();

            switch (input.valueType) {
            case assignWorkerReqType: {
                // start a fiber?
                {
                    auto output = do_assign_worker(input);
                    msg->setResponse(output);
                }
                break;
            }
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
}

void Shard::stop() {
    _channel->close();

    if (_loop_thread.isRunning()) {
        _loop_thread.join();
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Inner implements.
////////////////////////////////////////////////////////////////////////////////////////////////////////////

shardOp Shard::do_assign_worker(const shardOp& input) {
    auto req = std::static_pointer_cast<assignWorkerReq>(input.valuePtr);
    auto rsp = std::make_shared<assignWorkerRsp>();

    _worker->ServeShard(_shard_info);

    shardOp output{
        .valueType = addFeaturesRspType,
        .valuePtr = rsp,
    };
    return output;
};
shardOp Shard::do_add_features(const shardOp& input) {
    auto req = std::static_pointer_cast<addFeaturesReq>(input.valuePtr);
    auto rsp = std::make_shared<addFeaturesRsp>();

    // delegate to worker.
    auto ret = _worker->AddFeatures(_db_id, _shard_id, req->fts);
    if (ret == RetCode::RET_OK) {
        _shard_info.used += req->fts.size();
        _shard_mgr->UpdateShard(_shard_info);
    }

    // TODO: what about _worker api error?

    shardOp output{
        .valueType = addFeaturesRspType,
        .valuePtr = rsp,
    };
    return output;
};
shardOp Shard::do_search_feature(const shardOp& input) {
    auto req = std::static_pointer_cast<searchFeatureReq>(input.valuePtr);
    auto rsp = std::make_shared<searchFeatureRsp>();

    auto ret = _worker->SearchFeature(_shard_info.db_id, req->query, req->topk);
    rsp->fts.swap(ret);

    // TODO: what about _worker api error?

    shardOp output{
        .valueType = searchFeatureRspType,
        .valuePtr = rsp,
    };
    return output;
};

shardOp Shard::do_close_shard(const shardOp& input) {
    auto req = std::static_pointer_cast<closeShardReq>(input.valuePtr);
    auto rsp = std::make_shared<closeShardRsp>();

    _worker->CloseShard(_db_id, _shard_id);
    _shard_mgr->CloseShard(_db_id, _shard_id);

    // TODO: what about _worker api error?

    shardOp output{
        .valueType = closeShardRspType,
        .valuePtr = rsp,
    };
    return output;
};
