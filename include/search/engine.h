#pragma once

#include "nlohmann/json.hpp"
#include "search/searcher.h"
#include "types.h"

#include <map>

using namespace std;

using json = nlohmann::json;

namespace search {

    class BruteForceSearch : public Engine {

      public:
        BruteForceSearch(const json& config);
        ~BruteForceSearch() = default;

        RetCode TrainIndex() override;

        std::vector<Feature> Search(const Feature& query, size_t topK) override;

        std::vector<std::string> AddFeatures(const std::vector<Feature>& features) override;

        RetCode RemoveFeatures(const std::vector<std::string>& feature_ids) override;

      private:
        json _config;
    };

    class FaissSearch : public Engine {

      public:
        FaissSearch(const json& config);
        ~FaissSearch() = default;

        RetCode TrainIndex() override;

        std::vector<Feature> Search(const Feature& query, size_t topK) override;

        std::vector<std::string> AddFeatures(const std::vector<Feature>& features) override;

        RetCode RemoveFeatures(const std::vector<std::string>& feature_ids) override;

      private:
        json _config;
    };

} // namespace search
