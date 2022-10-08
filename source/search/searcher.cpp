#include "search/searcher.h"
#include "search/search_engine.h"

#include "types.h"
#include <cassert>
#include <memory>
#include <stdexcept>

namespace search {

    Searcher::Searcher(const json& config): _config(config) {
        if (!_config.contains("search_engine")) {
            throw std::invalid_argument("searcher config json doesnot contains `search_engine` field");
        }
        if (!_config.contains("storage_backend")) {
            throw std::invalid_argument("searcher config json doesnot contains `storage_backend` field");
        }
        if (_config["search_engine"] == SEARCH_ENGINE_BRUTE_FORCE) {
            _engine = std::make_shared<BruteForceSearch>(config);
        }
    };

    RetCode Searcher::Init() { return RetCode::RET_OK; };

    RetCode Searcher::Terminate() { return RetCode::RET_OK; };

    RetCode Searcher::Maintaince() { return RetCode::RET_OK; };

    std::vector<uint64> Searcher::AddFeatures(const std::vector<Feature>& features) {
        return {};
    };

    RetCode Searcher::RemoveFeatures(const std::vector<uint64>& feature_ids) {
        return RetCode::RET_OK;
    };

} // namespace search
