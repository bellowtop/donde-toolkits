#include "nlohmann/json.hpp"
#include "search/searcher.h"
#include "search/storage.h"
#include "types.h"

#include "uuid/uuid.h"

#include <filesystem>
#include <fstream>
#include <ios>
#include <map>
#include <opencv2/core/hal/interface.h>

using namespace std;

using json = nlohmann::json;

namespace search {

    FileSystemStorage::FileSystemStorage(const json& config)
        : _config(config),
          _db_dir(_config["path"]),
          _data_dir(_db_dir / "data"),
          _meta_dir(_db_dir / "meta") {

        std::filesystem::create_directories(_data_dir);
        std::filesystem::create_directories(_meta_dir);
    };

    std::vector<std::string> FileSystemStorage::AddFeatures(const std::vector<Feature>& features) {
        int count = features.size();
        std::vector<std::string> feature_ids;
        for (auto& ft : features) {
            std::string feature_id;
            feature_id.resize(32);

            uuid_t uuid;
            uuid_generate(uuid);
            uuid_unparse_lower(uuid, feature_id.data());

            std::cout << "uuid: " << feature_id << std::endl;

            auto filepath = _data_dir / (feature_id+".ft");
            try {
                std::ofstream file(filepath, std::ios::binary | std::ios::out);
                file << ft;
                feature_ids.push_back(feature_id);
            } catch(...) {
                std::cerr << "cannot save feature to " << filepath << std::endl;
                feature_ids.push_back("");
            }
        }

        return feature_ids;
    };

    std::vector<Feature> FileSystemStorage::LoadFeatures(const std::vector<std::string>& feature_ids) {
        int count = feature_ids.size();
        std::vector<Feature> features;
        for (auto& feature_id : feature_ids) {
            std::cout << "load feature_id: " << feature_id << std::endl;

            auto filepath = _data_dir / (feature_id+".ft");
            Feature ft;

            try {
                std::ifstream file(filepath, std::ios::binary | std::ios::in);
                file >> ft;
                ft.debugPrint();
                features.push_back(ft);
            } catch(...) {
                std::cerr << "cannot load feature, feature_path: " << filepath << std::endl;
                features.push_back(ft);
            }
        }

        return features;
    };

    RetCode FileSystemStorage::RemoveFeatures(const std::vector<std::string>& feature_ids) {
        for (auto& feature_id : feature_ids) {
            auto filepath = _data_dir / (feature_id+".ft");
            std::filesystem::remove(filepath);
        }
        return RetCode::RET_OK;
    };

} // namespace search
