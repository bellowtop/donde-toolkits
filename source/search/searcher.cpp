#include "search/searcher.h"

#include "search/engine.h"
#include "types.h"

#include <cassert>
#include <memory>
#include <stdexcept>

namespace search {

    Searcher::Searcher(const json& config) : _config(config) {
        if (!_config.contains("engine")) {
            throw std::invalid_argument("searcher config json doesnot contains `engine` field");
        }
        if (_config["engine"] == SEARCH_ENGINE_BRUTE_FORCE) {
            _engine = std::make_shared<BruteForceSearch>(_config["engine"]);
        }
    };

    RetCode Searcher::Init() { return RetCode::RET_OK; };

    RetCode Searcher::Terminate() { return RetCode::RET_OK; };

    RetCode Searcher::Maintaince() { return RetCode::RET_OK; };

    std::vector<std::string> Searcher::AddFeatures(const std::vector<Feature>& features) {
        return {};
    };

    RetCode Searcher::RemoveFeatures(const std::vector<std::string>& feature_ids) {
        return RetCode::RET_OK;
    };

} // namespace search
