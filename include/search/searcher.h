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

    const std::string SEARCH_ENGINE_BRUTE_FORCE = "brute_force";
    const std::string SEARCH_ENGINE_FAISS = "brute_force";

    const std::string STORAGE_BACKEND_FILE_SYSTEM = "file_system";
    const std::string STORAGE_BACKEND_CASSANDRA = "cassandra";

    class StorageBackend {

      public:
        StorageBackend();
        virtual ~StorageBackend() = default;

        virtual std::vector<uint64> AddFeatures(const std::vector<Feature>& features) = 0;

        virtual RetCode RemoveFeatures(const std::vector<uint64>& feature_ids) = 0;
    };

    class SearchEngine {

      public:
        SearchEngine();
        virtual ~SearchEngine() = default;

        virtual RetCode TrainIndex() = 0;

        virtual std::vector<Feature> Search(const Feature& query, size_t topK) = 0;

        virtual std::vector<uint64> AddFeatures(const std::vector<Feature>& features) = 0;

        virtual RetCode RemoveFeatures(const std::vector<uint64>& feature_ids) = 0;

      protected:
        std::shared_ptr<StorageBackend> _backend;
    };

    class Searcher {

      public:
        Searcher(const json& config);

        const json& GetConfig() const { return _config; };

        RetCode Init();

        RetCode Terminate();

        RetCode Maintaince();

        std::vector<uint64> AddFeatures(const std::vector<Feature>& features);

        RetCode RemoveFeatures(const std::vector<uint64>& feature_ids);

        std::vector<Feature> SearchFeature(const Feature& query, size_t topK);

      private:
        json _config;

        std::shared_ptr<SearchEngine> _engine;
    };

} // namespace search
