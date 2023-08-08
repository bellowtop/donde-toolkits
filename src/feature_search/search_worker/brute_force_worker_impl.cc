#include "brute_force_worker_impl.h"

#include "donde/definitions.h"
#include "donde/feature_search/feature_topk_rank.h"
#include "donde/feature_search/search_worker/brute_force_worker.h"
#include "mem_shard_impl.h"
#include "shard.h"
#include "shard_manager_impl.h"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <spdlog/spdlog.h>

using namespace std;

using json = nlohmann::json;

namespace donde_toolkits ::feature_search ::search_worker {

BruteForceWorkerImpl::BruteForceWorkerImpl(Driver& driver)
    : ISearchWorker(), _shard_mgr(std::make_unique<ShardManagerImpl>(driver)), _driver(driver){};

BruteForceWorkerImpl::~BruteForceWorkerImpl(){};

RetCode BruteForceWorkerImpl::ServeShards(const std::vector<DBShard>& shard_infos) {
    for (auto& s : shard_infos) {
        // remove ShardImpl dependency ?
        Shard* shard = new MemoryShardImpl(*_shard_mgr, _driver, s);
        _shard_mgr->ManageShard(s.db_id, shard);
    }
    return RetCode::RET_OK;
};

RetCode BruteForceWorkerImpl::CloseShards(const std::vector<DBShard>& shard_infos) {
    for (auto& s : shard_infos) {
        _shard_mgr->CloseShard(s.db_id, s.shard_id);
    }
    return RetCode::RET_OK;
};

std::vector<FeatureSearchItem>
BruteForceWorkerImpl::SearchFeature(const std::string& db_id, const Feature& query, size_t topk) {
    auto shards = _shard_mgr->ListShards(db_id);
    if (shards.empty()) {
        spdlog::error("search err: no shards in db[{}]", db_id);
        return {};
    }

    FeatureTopkRanking rank(query, topk);

    // search across evey shard, and merge results.
    for (auto& s : shards) {
        std::vector<FeatureSearchItem> searched = s->SearchFeature(query, topk);
        // merge
        rank.FeedIn(searched);
    }

    std::vector<FeatureSearchItem> ret = rank.SortOut();

    // search with max-heap.
    return ret;
};

std::vector<std::string>
BruteForceWorkerImpl::AddFeatures(const std::string& db_id,
                                  const std::vector<FeatureDbItem>& features) {
    Shard* shard = _shard_mgr->FindShard(db_id);
    if (!shard) {
        spdlog::error("cannot find a writable shard for this db!");
        return {};
    }

    // what about shard full? the search_manager handle this.
    std::vector<std::string> feature_ids = shard->AddFeatures(features);
    return feature_ids;
};

RetCode BruteForceWorkerImpl::RemoveFeatures(const std::string& db_id,
                                             const std::vector<std::string>& feature_ids) {

    Shard* shard = _shard_mgr->FindShard(db_id);
    return shard->RemoveFeatures(feature_ids);
};

} // namespace donde_toolkits::feature_search::search_worker
