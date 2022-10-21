#include "nlohmann/json.hpp"
#include "search/searcher.h"
#include "search/storage.h"
#include "types.h"
#include "uuid/uuid.h"

#include <exception>
#include <filesystem>
#include <fstream>
#include <ios>
#include <iterator>
#include <map>
#include <msgpack.hpp>
#include <opencv2/core/hal/interface.h>
#include <sstream>

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

            auto filepath = _data_dir / (feature_id + ".ft");
            try {
                std::ofstream file(filepath, std::ios::binary | std::ios::out);
                std::stringstream ss;
                msgpack::pack(ss, ft);

                // write to file
                std::string data(ss.str());
                file << data;

                // std::cout << "write data.size" << data.size() << std::endl;
                feature_ids.push_back(feature_id);
            } catch (...) {
                std::cerr << "cannot save feature to " << filepath << std::endl;
                feature_ids.push_back("");
            }
        }

        return feature_ids;
    };

    std::vector<Feature>
    FileSystemStorage::LoadFeatures(const std::vector<std::string>& feature_ids) {
        int count = feature_ids.size();
        std::vector<Feature> features;
        for (auto& feature_id : feature_ids) {
            std::cout << "load feature_id: " << feature_id << std::endl;

            auto filepath = _data_dir / (feature_id + ".ft");
            try {
                // read to string
                std::ifstream file(filepath, std::ios::binary | std::ios::in);
                std::string data = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
                // std::cout << "filepath: " << filepath << std::endl;
                // std::cout << "data.size(): " << data.size() << std::endl;
                auto oh = msgpack::unpack(data.data(), data.size());
                Feature ft = oh.get().as<Feature>();
                ft.debugPrint();

                features.push_back(ft);
            } catch (const std::exception& exc) {
                std::cerr << "cannot load feature, feature_path: " << filepath << exc.what() << std::endl;
                features.push_back(Feature{});
            }
        }

        return features;
    };

    RetCode FileSystemStorage::RemoveFeatures(const std::vector<std::string>& feature_ids) {
        for (auto& feature_id : feature_ids) {
            auto filepath = _data_dir / (feature_id + ".ft");
            std::filesystem::remove(filepath);
        }
        return RetCode::RET_OK;
    };

} // namespace search
