#pragma once

#include "faiss/Index2Layer.h"
#include "nlohmann/json.hpp"
#include "types.h"

#include <map>
#include <memory>
#include <opencv2/core/hal/interface.h>

using namespace std;

using json = nlohmann::json;

// TODO: refactor with MessagePack?
inline istream& operator>>(istream& is, Feature& ft) {
    is >> ft.model;
    is >> ft.version;
    is >> ft.dimension;

    std::string blob;
    blob.resize(ft.dimension * sizeof(float));

    char* p = reinterpret_cast<char*>(blob.data());
    size_t size = blob.size();
    is.read(p, size);

    ft.raw = convertFeatureBlobToFloats(blob);

    return is;
}

inline ostream& operator<<(ostream& os, const Feature& ft) {
    os << ft.model;
    os << ft.version;
    os << ft.dimension;

    const char* p = reinterpret_cast<const char*>(ft.raw.data());
    size_t size = sizeof(float) * ft.raw.size();

    // std::string str;
    // str.resize(size);
    // std::copy(p, p + size, str.data());
    // os << str;

    os.write(p, size);
    // os.flush();

    return os;
}



namespace search {

    const std::string SEARCH_ENGINE_BRUTE_FORCE = "brute_force";
    const std::string SEARCH_ENGINE_FAISS = "brute_force";

    const std::string STORAGE_BACKEND_FILE_SYSTEM = "file_system";
    const std::string STORAGE_BACKEND_CASSANDRA = "cassandra";

    class Storage {

      public:
        Storage() = default;
        virtual ~Storage() = default;

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
