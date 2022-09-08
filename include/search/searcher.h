#pragma once

#include "faiss/Index2Layer.h"
#include "nlohmann/json.hpp"
#include "types.h"

#include <map>
#include <memory>
#include <opencv2/core/hal/interface.h>

using namespace std;

using json = nlohmann::json;

namespace search {

    class StorageBackend {
      public:
        StorageBackend();

        virtual RetCode AddFeatures(const std::vector<Feature>& features) = 0;

        virtual RetCode RemoveFeatures(const std::vector<Feature>& features) = 0;

      protected:
        virtual ~StorageBackend(){};
    };

    class SearchEngine {
      public:
        SearchEngine(std::shared_ptr<StorageBackend> backend) : _backend(backend){};

        virtual RetCode TrainIndex() = 0;
        virtual std::vector<Feature> Search(const Feature& query, size_t topK) = 0;

      protected:
        virtual ~SearchEngine(){};

      protected:
        std::shared_ptr<StorageBackend> _backend;
    };

    class Searcher {

      public:
        Searcher(const json& config);
        const json& GetConfig() { return _config; };

        RetCode Init();

        RetCode Terminate();

        RetCode Maintaince();

        RetCode AddFeatures(const std::vector<Feature>& features, std::vector<uint64>& feature_ids);

        RetCode RemoveFeatures(const std::vector<Feature>& features);

        std::vector<Feature> SearchFeature(const Feature& query, size_t topK);

      private:
        json _config;

        std::shared_ptr<SearchEngine> _engine;
        std::shared_ptr<StorageBackend> _backend;
    };

} // namespace search
