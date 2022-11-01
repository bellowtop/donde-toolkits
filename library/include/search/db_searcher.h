#pragma once

#include "definitions.h"
#include "driver.h"
#include "faiss/Index2Layer.h"
#include "nlohmann/json.hpp"
#include "types.h"

#include <map>
#include <memory>
#include <opencv2/core/hal/interface.h>

using namespace std;

using json = nlohmann::json;

namespace search {

    class Searcher {

      public:
        ~Searcher() = default;

        RetCode Init();

        RetCode Terminate();

        RetCode TrainIndex();

        std::vector<std::string> AddFeatures(const std::vector<FeatureDbItem>& features);

        RetCode RemoveFeatures(const std::vector<std::string>& feature_ids);

        std::vector<FeatureSearchItem> SearchFeature(const Feature& query, size_t topk);
    };

} // namespace search
