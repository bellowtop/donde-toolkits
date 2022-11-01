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
            _engine = std::make_shared<BruteForceSearch>(_config[SEARCH_ENGINE_BRUTE_FORCE]);
        }
    };

    RetCode Searcher::Init() { return RetCode::RET_OK; };

    RetCode Searcher::Terminate() { return RetCode::RET_OK; };

    RetCode Searcher::TrainIndex() {
        _engine->TrainIndex();
        return RetCode::RET_OK;
    };

    std::vector<std::string> Searcher::AddFeatures(const std::vector<FeatureDbItem>& features) {
        return _engine->AddFeatures(features);
    };

    RetCode Searcher::RemoveFeatures(const std::vector<std::string>& feature_ids) {
        return _engine->RemoveFeatures(feature_ids);
    };

    std::vector<FeatureSearchItem> Searcher::SearchFeature(const Feature& query, size_t topk) {
        return _engine->Search(query, topk);
    }

} // namespace search
