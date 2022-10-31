#include "nlohmann/json.hpp"
#include "search/searcher.h"
#include "search/storage.h"
#include "types.h"
#include "uuid/uuid.h"

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <ios>
#include <iterator>
#include <map>
#include <memory>
#include <msgpack.hpp>
#include <opencv2/core/hal/interface.h>
#include <sqlite3.h>
#include <sstream>
#include <stdexcept>

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

        std::string db_filepath = _meta_dir / "sqlite3.db";

        try {
            _meta_db = std::make_unique<SQLite::Database>(db_filepath, SQLite::OPEN_READWRITE
                                                                           | SQLite::OPEN_CREATE);
        } catch (std::exception& exc) {
            spdlog::error("cannot open database: ", exc.what());
            std::abort();
        }
    };

    RetCode FileSystemStorage::Init() {
        // init table.
        return init_features_meta_db(_meta_db.get());
    };

    PageData<FeatureDbItemList> FileSystemStorage::ListFeatures(uint page, uint perPage) {
        uint64 count = count_features_in_meta_db(_meta_db.get());
        if (count <= 0) {
            spdlog::warn("FileSystemStorage::ListFeatures, count is {}", count);
            return {};
        }

        uint64 totalPage = (count + perPage - 1) / perPage;

        std::vector<FeatureDbItem> feature_ids
            = list_features_from_meta_db(_meta_db.get(), page * perPage, perPage);
        spdlog::debug("feature_ids size: ", feature_ids.size());

        PageData<FeatureDbItemList> ret{uint64(page), uint64(perPage), totalPage, feature_ids};
        return ret;
    };

    std::vector<std::string> FileSystemStorage::AddFeatures(const std::vector<FeatureDbItem>& features) {
        int count = features.size();
        std::vector<std::string> feature_ids;
        std::vector<std::string> metadatas;

        for (auto& item : features) {
            auto ft = item.feature;

            std::string feature_id;
            feature_id.resize(32);

            uuid_t uuid;
            uuid_generate(uuid);
            uuid_unparse_lower(uuid, feature_id.data());

            // std::cout << "uuid: " << feature_id << std::endl;

            auto filepath = _data_dir / (feature_id + ".ft");
            try {
                std::ofstream file(filepath, std::ios::binary | std::ios::out);
                std::stringstream ss;
                msgpack::pack(ss, ft);

                // write to file
                std::string data(ss.str());
                file << data;

                json j(item.metadata); // must use (), while not {}
                std::string meta_str{ j.dump() };
                metadatas.push_back(meta_str);

                // std::cout << "write data.size" << data.size() << std::endl;
                feature_ids.push_back(feature_id);

            } catch (const std::exception& exc) {
                spdlog::error("cannot save feature to : {}, exc: {}", filepath.string(), exc.what());
                feature_ids.push_back("");
                metadatas.push_back("{}");
            }
        }

        // insert feature to meta db.
        insert_features_to_meta_db(_meta_db.get(), feature_ids, metadatas);

        return feature_ids;
    };

    std::vector<Feature>
    FileSystemStorage::LoadFeatures(const std::vector<std::string>& feature_ids) {
        int count = feature_ids.size();
        std::vector<Feature> features;

        for (auto& feature_id : feature_ids) {
            spdlog::debug("load feature_id: {}", feature_id);

            auto filepath = _data_dir / (feature_id + ".ft");
            try {
                // read to string
                std::ifstream file(filepath, std::ios::binary | std::ios::in);
                std::string data = std::string(std::istreambuf_iterator<char>(file),
                                               std::istreambuf_iterator<char>());

                // std::cout << "filepath: " << filepath << std::endl;
                // std::cout << "data.size(): " << data.size() << std::endl;
                auto oh = msgpack::unpack(data.data(), data.size());
                Feature ft = oh.get().as<Feature>();
                // ft.debugPrint();

                features.push_back(ft);
            } catch (const std::exception& exc) {
                spdlog::error("cannot load feature, feature_path: {}, exc: {}", filepath.string(), exc.what());
                features.push_back({});
            }
        }

        return features;
    };

    RetCode FileSystemStorage::RemoveFeatures(const std::vector<std::string>& feature_ids) {
        if (feature_ids.size() == 0) {
            return RetCode::RET_OK;
        }

        for (auto& feature_id : feature_ids) {
            auto filepath = _data_dir / (feature_id + ".ft");
            std::filesystem::remove(filepath);
        }

        return delete_features_from_meta_db(_meta_db.get(), feature_ids);
    };

} // namespace search
