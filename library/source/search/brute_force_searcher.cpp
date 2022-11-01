#include "nlohmann/json.hpp"
#include "search/db_searcher.h"
#include "search/driver.h"
#include "types.h"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <queue>

using namespace std;

using json = nlohmann::json;

namespace search {

    class BruteForceSearcher : public Searcher {

      public:
        BruteForceSearcher(const json& config, Driver& driver);

        ~BruteForceSearcher() = default;

        RetCode TrainIndex() override;

        std::vector<FeatureSearchItem> Search(const Feature& query, size_t topK) override;

        std::vector<std::string> AddFeatures(const std::vector<FeatureDbItem>& features) override;

        RetCode RemoveFeatures(const std::vector<std::string>& feature_ids) override;

      private:
        json _config;
        Driver& _storage;
    };

    BruteForceSearcher::BruteForceSearcher(const json& config, Driver& driver)
        : _config(config), _storage(driver){};

    RetCode BruteForceSearcher::TrainIndex() {
        spdlog::warn("TrainIndex is not implemented by BruteForceSearch");
        return RetCode::RET_OK;
    };

    std::vector<FeatureSearchItem> BruteForceSearcher::Search(const Feature& query, size_t topk) {

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

        std::vector<FeatureSearchItem> ret;

        uint page = 0;
        uint perPage = 10;
        auto pageData = _storage.ListFeatures(page, perPage);

        std::priority_queue<featureScore, std::vector<featureScore>, featureScoreComparator>
            min_heap;

        while (pageData.data.size() != 0) {
            // feed in min-heap
            std::vector<std::string> feature_ids = search::convert_to_feature_ids(pageData.data);
            std::vector<Feature> fts = _storage.LoadFeatures(feature_ids);
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
            pageData = _storage.ListFeatures(++page, perPage);
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
    BruteForceSearcher::AddFeatures(const std::vector<FeatureDbItem>& features) {
        std::vector<std::string> feature_ids = _storage.AddFeatures(features);
        return feature_ids;
    };

    RetCode BruteForceSearcher::RemoveFeatures(const std::vector<std::string>& feature_ids) {
        _storage.RemoveFeatures(feature_ids);
        return RetCode::RET_OK;
    };

} // namespace search
