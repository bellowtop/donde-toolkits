#include "nlohmann/json.hpp"
#include "search/engine.h"
#include "search/searcher.h"
#include "search/storage.h"
#include "types.h"

#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <algorithm>

using namespace std;

using json = nlohmann::json;

namespace search {

    BruteForceSearch::BruteForceSearch(const json& config) : _config(config) {
        if (!_config.contains("storage")) {
            throw std::invalid_argument(
                "search engine config json doesnot contains `storage` field");
        }
        _storage = std::make_shared<FileSystemStorage>(_config["storage"]);
        _storage->Init();
    }

    RetCode BruteForceSearch::TrainIndex() {
        spdlog::warn("TrainIndex is not implemented by BruteForceSearch");
        return RetCode::RET_OK;
    };

    std::vector<FeatureSearchResult> BruteForceSearch::Search(const Feature& query,
                                                                      size_t topk) {

        struct FeatureScore {
            float score;
            Feature query;
            Feature target;
        };

        struct FeatureScoreComparator {
            bool operator()(const FeatureScore& lhs, const FeatureScore& rhs) {
                return lhs.score > rhs.score;
            };
        };

        std::vector<FeatureSearchResult> ret;

        uint page = 0;
        uint perPage = 10;
        auto pageData = _storage->ListFeautreIDs(page, perPage);

        std::priority_queue<FeatureScore, std::vector<FeatureScore>, FeatureScoreComparator>
            min_heap;

        while (pageData.data.size() != 0) {
            // feed in min-heap
            std::vector<Feature> fts = _storage->LoadFeatures(pageData.data);
            for (auto& ft : fts) {
                float score = ft.compare(query);
                FeatureScore target{score, query, ft};

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
            pageData = _storage->ListFeautreIDs(++page, perPage);
        }

        while (!min_heap.empty()) {
            const FeatureScore& score = min_heap.top();
            min_heap.pop();

            ret.push_back(FeatureSearchResult{score.target, score.score});
        }

        std::reverse(ret.begin(), ret.end());

        // search with max-heap.
        return ret;
    };

    std::vector<std::string> BruteForceSearch::AddFeatures(const std::vector<Feature>& features) {
        std::vector<std::string> feature_ids = _storage->AddFeatures(features);
        return feature_ids;
    };

    RetCode BruteForceSearch::RemoveFeatures(const std::vector<std::string>& feature_ids) {
        _storage->RemoveFeatures(feature_ids);
        return RetCode::RET_OK;
    };

} // namespace search
