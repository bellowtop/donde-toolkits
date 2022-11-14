#include "brute_force_searcher.h"

#include "definitions.h"
#include "nlohmann/json.hpp"
#include "search/driver.h"

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
    BruteForceSearcher(Driver& driver);

    ~BruteForceSearcher() = default;

    RetCode Init() override {
        spdlog::warn("Init is not implemented by BruteForceSearch");
        return RetCode::RET_OK;
    };

    RetCode Terminate() override {
        spdlog::warn("Iterminate is not implemented by BruteForceSearch");
        return RetCode::RET_OK;
    };

    RetCode TrainIndex() override {
        spdlog::warn("TrainIndex is not implemented by BruteForceSearch");
        return RetCode::RET_OK;
    };

    std::vector<FeatureSearchItem> SearchFeature(const std::string& db_id, const Feature& query,
                                                 size_t topk) override;

    std::vector<std::string> AddFeatures(const std::string& db_id,
                                         const std::vector<FeatureDbItem>& features) override;

    RetCode RemoveFeatures(const std::string& db_id,
                           const std::vector<std::string>& feature_ids) override;

  private:
    Driver& _driver;
};

BruteForceSearcher::BruteForceSearcher(Driver& driver) : _driver(driver){};

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

    std::vector<FeatureSearchItem> ret;

    uint page = 0;
    uint perPage = 10;
    auto pageData = _driver.ListFeatures(db_id, page, perPage);

    std::priority_queue<featureScore, std::vector<featureScore>, featureScoreComparator> min_heap;

    while (pageData.data.size() != 0) {
        // feed in min-heap
        std::vector<std::string> feature_ids = search::convert_to_feature_ids(pageData.data);
        std::vector<Feature> fts = _driver.LoadFeatures(db_id, feature_ids);
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
        pageData = _driver.ListFeatures(db_id, ++page, perPage);
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
    std::vector<std::string> feature_ids = _driver.AddFeatures(db_id, features);
    return feature_ids;
};

RetCode BruteForceSearcher::RemoveFeatures(const std::string& db_id,
                                           const std::vector<std::string>& feature_ids) {
    _driver.RemoveFeatures(db_id, feature_ids);
    return RetCode::RET_OK;
};

} // namespace search
