#include "search_manager/shard.h"

#include "search/definitions.h"
#include "search_manager/shard_manager.h"
#include "types.h"
#include "utils.h"

#include <spdlog/spdlog.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Shard
////////////////////////////////////////////////////////////////////////////////////////////////////////////

// TODO: async these operations. maybe a controller and a task queue ?

// Assign a worker for this shard.
RetCode Shard::AssignWorker(Worker* worker) {
    if (_worker != nullptr) {
        spdlog::error("shard already has worker, double AssignWorker??.");
        return RetCode::RET_ERR;
    }

    if (worker != nullptr) {
        _worker = worker;
        _worker_id = worker->GetWorkerID();
    }

    return _worker->ServeShard(_shard_info);
};

// AddFeatures to this shard, delegate to worker client to do the actual storage.
RetCode Shard::AddFeatures(std::vector<Feature> fts) {
    if (_worker == nullptr) {
        spdlog::error("shard has no worker, AssignWorker first.");
        return RetCode::RET_ERR;
    }

    // delegate to worker.
    auto ret = _worker->AddFeatures(_db_id, _shard_id, fts);
    if (ret == RetCode::RET_OK) {
        _shard_info.used += fts.size();
        _shard_mgr->UpdateShard(_shard_info);
        return ret;
    }

    return RetCode::RET_ERR;
};

// SearchFeature in this shard, delegate to worker client to do the actual search.
std::vector<FeatureScore> Shard::SearchFeature(const Feature& query, int topk) {
    if (_worker == nullptr) {
        spdlog::error("shard has no worker, AssignWorker first.");
        return {};
    }

    return _worker->SearchFeature(_shard_info.db_id, query, topk);
};

RetCode Shard::Close() {
    _worker->CloseShard(_db_id, _shard_id);

    _shard_mgr->CloseShard(_db_id, _shard_id);

    _shard_info.is_closed = true;

    return {};
};
