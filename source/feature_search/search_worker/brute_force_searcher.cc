#include "brute_force_searcher.h"

#include "donde/definitions.h"
#include "donde/feature_search/search_worker/shard.h"
#include "mem_shard_impl.h"
#include "nlohmann/json.hpp"
#include "source/feature_search/feature_topk_rank.h"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <spdlog/spdlog.h>

using namespace std;

using json = nlohmann::json;

namespace donde {

namespace feature_search {

namespace search_worker {

BruteForceSearcher::BruteForceSearcher(ShardManager* shard_manager, Driver* driver)
    : Searcher(shard_manager), _shard_mgr(shard_manager), _driver(driver){};

RetCode BruteForceSearcher::ServeShards(const std::vector<DBShard>& shard_infos) {
    for (auto& s : shard_infos) {
        // remove ShardImpl dependency ?
        Shard* shard = new MemoryShardImpl(*_shard_mgr, *_driver, s);
        _shard_mgr->ManageShard(s.db_id, shard);
    }
    return RetCode::RET_OK;
};

RetCode BruteForceSearcher::CloseShards(const std::vector<DBShard>& shard_infos) {
    for (auto& s : shard_infos) {
        _shard_mgr->CloseShard(s.db_id, s.shard_id);
    }
    return RetCode::RET_OK;
};

std::vector<FeatureSearchItem>
BruteForceSearcher::SearchFeature(const std::string& db_id, const Feature& query, size_t topk) {
    auto shards = _shard_mgr->ListShards(db_id);

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
BruteForceSearcher::AddFeatures(const std::string& db_id,
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

RetCode BruteForceSearcher::RemoveFeatures(const std::string& db_id,
                                           const std::vector<std::string>& feature_ids) {

    Shard* shard = _shard_mgr->FindShard(db_id);
    return shard->RemoveFeatures(feature_ids);
};

} // namespace search_worker

} // namespace feature_search

} // namespace donde
