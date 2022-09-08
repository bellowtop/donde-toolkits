#include "search/searcher.h"

#include "types.h"

namespace search {
    Searcher::Searcher(const json& config){};
    RetCode Searcher::Init() { return RetCode::RET_OK; };
    RetCode Searcher::Terminate() { return RetCode::RET_OK; };
    RetCode Searcher::Maintaince() { return RetCode::RET_OK; };
    RetCode Searcher::AddFeatures(const std::vector<Feature>& features,
                                  std::vector<uint64>& feature_ids) {
        return RetCode::RET_OK;
    };
} // namespace search
