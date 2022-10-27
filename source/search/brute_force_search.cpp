#include "nlohmann/json.hpp"
#include "search/engine.h"
#include "search/searcher.h"
#include "search/storage.h"
#include "types.h"

#include <map>
#include <memory>

using namespace std;

using json = nlohmann::json;

namespace search {

    BruteForceSearch::BruteForceSearch(const json& config) : _config(config) {
        if (!_config.contains("storage")) {
            throw std::invalid_argument(
                "search engine config json doesnot contains `storage` field");
        }
        _storage = std::make_shared<FileSystemStorage>(_config["storage"]);
    }

    RetCode BruteForceSearch::TrainIndex() {
        std::cout << "TrainIndex is not implemented by BruteForceSearch" << std::endl;
        return RetCode::RET_OK;
    };

    std::vector<Feature> BruteForceSearch::Search(const Feature& query, size_t topK) {
        uint page = 0;
        uint perPage = 10;
        auto pageData = _storage->ListFeautreIDs(page, perPage);

        while (pageData.data.size() != 0) {
            // feed in max-heap

            // read next page
            pageData = _storage->ListFeautreIDs(++page, perPage);
        }

        // search with max-heap.
        return std::vector<Feature>();
    };

    std::vector<std::string> BruteForceSearch::AddFeatures(const std::vector<Feature>& features) {
        std::vector<std::string> feature_ids = _storage->AddFeatures(features);
        // cache it to local memory
        return feature_ids;
    };

    RetCode BruteForceSearch::RemoveFeatures(const std::vector<std::string>& feature_ids) {
        _storage->RemoveFeatures(feature_ids);
        return RetCode::RET_OK;
    };

} // namespace search
