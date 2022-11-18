#include "brute_force_searcher.h"

#include "donde/definitions.h"
#include "donde/feature_search/search_worker/shard.h"
#include "mem_shard_impl.h"
#include "nlohmann/json.hpp"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <queue>

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
        Shard* shard = new MemoryShardImpl(*_shard_mgr, s);
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

    struct featureScore {
        float score;
        Feature query;
        Feature target;
    };

    struct featureScoreComparator {
        bool operator()(const featureScore& lhs, const featureScore& rhs) {
            return lhs.score > rhs.score;
        };
    };

    auto shards = _shard_mgr->ListShards(db_id);
    for (auto& s : shards) {
        std::vector<FeatureScore> ret = s->SearchFeature(query, topk);
    }

    std::vector<FeatureSearchItem> ret;

    uint page = 0;
    uint perPage = 10;
    auto pageData = _driver->ListFeatures(db_id, page, perPage);

    std::priority_queue<featureScore, std::vector<featureScore>, featureScoreComparator> min_heap;

    while (pageData.data.size() != 0) {
        // feed in min-heap
        std::vector<std::string> feature_ids = convert_to_feature_ids(pageData.data);
        std::vector<Feature> fts = _driver->LoadFeatures(db_id, feature_ids);
        for (auto& ft : fts) {
            float score = ft.compare(query);
            featureScore target{score, query, ft};

            if (min_heap.size() < topk) {
                min_heap.push(target);
                continue;
            }
            if (min_heap.top().score < target.score) {
                min_heap.pop();
                min_heap.push(target);
            }
        }

        // read next page, caution: copy assignment here!
        pageData = _driver->ListFeatures(db_id, ++page, perPage);
    }

    while (!min_heap.empty()) {
        const featureScore& score = min_heap.top();
        min_heap.pop();

        ret.push_back(FeatureSearchItem{score.target, score.score});
    }

    std::reverse(ret.begin(), ret.end());

    // search with max-heap.
    return ret;
};

std::vector<std::string>
BruteForceSearcher::AddFeatures(const std::string& db_id,
                                const std::vector<FeatureDbItem>& features) {
    std::vector<std::string> feature_ids = _driver->AddFeatures(db_id, features);
    return feature_ids;
};

RetCode BruteForceSearcher::RemoveFeatures(const std::string& db_id,
                                           const std::vector<std::string>& feature_ids) {
    _driver->RemoveFeatures(db_id, feature_ids);
    return RetCode::RET_OK;
};

} // namespace search_worker

} // namespace feature_search

} // namespace donde
