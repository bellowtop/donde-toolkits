#pragma once

#include "nlohmann/json.hpp"
#include "search/searcher.h"
#include "types.h"

#include <map>

using namespace std;

using json = nlohmann::json;

namespace search {

    class BruteForceSearch : public SearchEngine {

      public:
        BruteForceSearch(const json& config);
        ~BruteForceSearch() = default;

        RetCode TrainIndex() override;

        std::vector<Feature> Search(const Feature& query, size_t topK) override;

        std::vector<uint64> AddFeatures(const std::vector<Feature>& features) override;

        RetCode RemoveFeatures(const std::vector<uint64>& feature_ids) override;

      private:
        json _config;
    };

    class FaissSearch : public SearchEngine {

      public:
        FaissSearch(const json& config);
        ~FaissSearch() = default;

        RetCode TrainIndex() override;

        std::vector<Feature> Search(const Feature& query, size_t topK) override;

        std::vector<uint64> AddFeatures(const std::vector<Feature>& features) override;

        RetCode RemoveFeatures(const std::vector<uint64>& feature_ids) override;

      private:
        json _config;
    };

} // namespace search
