#pragma once

#include "faiss/Index2Layer.h"
#include "nlohmann/json.hpp"
#include "types.h"

#include <map>
#include <memory>

using namespace std;

using json = nlohmann::json;

namespace search {

    class SearchEngine {};

    class StorageBackend {};

    class Searcher {
      public:
        Searcher(const json& config);

        const json& GetConfig() { return _config; };

        RetCode Init();

        RetCode Terminate();

        RetCode Maintaince();

        RetCode AddFeatures(const vector<Feature>& features);

        RetCode SearchFeature(const Feature query, size_t topK);

      private:
        json _config;

        std::shared_ptr<StorageBackend> backend;
        std::shared_ptr<SearchEngine> engine;
    };
} // namespace search
