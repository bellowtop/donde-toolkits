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

    template<typename T>
    struct PageData {
        uint64 page;
        uint64 perPage;
        uint64 totalPage;
        T data;

        PageData() {}

        PageData(uint64 _page, uint64 _perPage, uint64 _totalPage, T _data):
            page(_page), perPage(_perPage), totalPage(_totalPage), data(_data)
        {}

        PageData& operator=(const PageData& lhs) {
            page = lhs.page;
            perPage = lhs.perPage;
            totalPage = lhs.totalPage;
            data = lhs.data;
            return *this;
        };
    };

    using FeatureIDList = std::vector<std::string>;

    const std::string SEARCH_ENGINE_BRUTE_FORCE = "brute_force";
    const std::string SEARCH_ENGINE_FAISS = "brute_force";

    const std::string STORAGE_BACKEND_FILE_SYSTEM = "file_system";
    const std::string STORAGE_BACKEND_CASSANDRA = "cassandra";

    class Storage {

      public:
        Storage() = default;
        virtual ~Storage() = default;

        virtual RetCode Init() = 0;

        virtual PageData<FeatureIDList> ListFeautreIDs(uint start, uint limit) = 0;

        virtual std::vector<std::string> AddFeatures(const std::vector<Feature>& features) = 0;

        virtual std::vector<Feature> LoadFeatures(const std::vector<std::string>& feature_ids) = 0;

        virtual RetCode RemoveFeatures(const std::vector<std::string>& feature_ids) = 0;
    };

    class Engine {

      public:
        Engine() = default;
        virtual ~Engine() = default;

        virtual RetCode TrainIndex() = 0;

        virtual std::vector<Feature> Search(const Feature& query, size_t topK) = 0;

        virtual std::vector<std::string> AddFeatures(const std::vector<Feature>& features) = 0;

        virtual RetCode RemoveFeatures(const std::vector<std::string>& feature_ids) = 0;

      protected:
        std::shared_ptr<Storage> _storage;
    };

    class Searcher {

      public:
        Searcher(const json& config);
        ~Searcher() = default;

        const json& GetConfig() const { return _config; };

        RetCode Init();

        RetCode Terminate();

        RetCode Maintaince();

        std::vector<std::string> AddFeatures(const std::vector<Feature>& features);

        RetCode RemoveFeatures(const std::vector<std::string>& feature_ids);

        std::vector<Feature> SearchFeature(const Feature& query, size_t topK);

      private:
        json _config;

        std::shared_ptr<Engine> _engine;
    };

} // namespace search
