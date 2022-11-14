#pragma once

#include "definitions.h"
#include "driver.h"
#include "faiss/Index2Layer.h"
#include "nlohmann/json.hpp"

#include <map>
#include <memory>
#include <opencv2/core/hal/interface.h>

using namespace std;

using json = nlohmann::json;

namespace donde {

namespace feature_search {

class Searcher {

  public:
    virtual ~Searcher() = default;

    virtual RetCode Init() = 0;

    virtual RetCode Terminate() = 0;

    virtual RetCode TrainIndex() = 0;

    virtual std::vector<std::string> AddFeatures(const std::string& db_id,
                                                 const std::vector<FeatureDbItem>& features)
        = 0;

    virtual RetCode RemoveFeatures(const std::string& db_id,
                                   const std::vector<std::string>& feature_ids)
        = 0;

    virtual std::vector<FeatureSearchItem> SearchFeature(const std::string& db_id,
                                                         const Feature& query, size_t topk)
        = 0;
};

} // namespace feature_search
} // namespace donde
