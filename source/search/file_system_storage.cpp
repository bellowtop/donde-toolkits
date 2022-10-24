#include "nlohmann/json.hpp"
#include "search/searcher.h"
#include "search/storage.h"
#include "types.h"
#include "uuid/uuid.h"

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
        auto db = init_features_meta_db(db_filepath);

        // all set.
        _meta_db = unique_sqlite3(db);
    };

    std::vector<std::string> FileSystemStorage::ListFeautreIDs(int start, int limit) {
        return list_features_from_meta_db(start, limit);
    };

    RetCode FileSystemStorage::Init() { return RetCode::RET_OK; };

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

            } catch (const std::exception& exc) {
                std::cerr << "cannot save feature to " << filepath << ", exc: " << exc.what()
                          << std::endl;
                feature_ids.push_back("");
            }
        }

        // insert feature to meta db.
        insert_features_to_meta_db(feature_ids);

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
                std::string data = std::string(std::istreambuf_iterator<char>(file),
                                               std::istreambuf_iterator<char>());

                // std::cout << "filepath: " << filepath << std::endl;
                // std::cout << "data.size(): " << data.size() << std::endl;
                auto oh = msgpack::unpack(data.data(), data.size());
                Feature ft = oh.get().as<Feature>();
                ft.debugPrint();

                features.push_back(ft);
            } catch (const std::exception& exc) {
                std::cerr << "cannot load feature, feature_path: " << filepath << exc.what()
                          << std::endl;
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

    // meta db implements
    sqlite3* FileSystemStorage::init_features_meta_db(std::string db_filepath) {
        sqlite3* db;

        int ret = sqlite3_open(db_filepath.c_str(), &db);
        if (ret != SQLITE_OK) {
            throw std::runtime_error("cannot open meta.db sqlite3 database");
        }

        // now create some table
        std::string sql = "create table if not exists features("
                          "id int primary key not null, "
                          "feature_id char(64)"
                          ");";

        char* error_msg;
        ret = sqlite3_exec(db, sql.c_str(), nullptr, 0, &error_msg);
        if (ret != SQLITE_OK) {
            std::cout << "sqlite3 create table err: " << error_msg << std::endl;
            sqlite3_free(error_msg);
            sqlite3_close_v2(db);
            throw std::runtime_error("cannot open create tables in sqlite3 database");
        }

        return db;
    }

    std::vector<std::string> FileSystemStorage::list_features_from_meta_db(int start, int limit) {
        std::vector<std::string> feature_ids;

        std::string sql = "select * from features offset ?1 limit ?2 ";
        sqlite3_stmt* stmt = nullptr;
        int ret;

        ret = sqlite3_prepare_v2(_meta_db.get(), sql.c_str(), -1, &stmt, nullptr);
        if (ret != SQLITE_OK) {
            std::cout << "error prepare statement" << std::endl;
            return feature_ids;
        }
        ret = sqlite3_bind_int(stmt, 1, start);
        if (ret != SQLITE_OK) {
            std::cout << "error bind offset ?1 to start: " << start << std::endl;
            return feature_ids;
        }
        ret = sqlite3_bind_int(stmt, 2, limit);
        if (ret != SQLITE_OK) {
            std::cout << "error bind offset ?2 to limit: " << start << std::endl;
            return feature_ids;
        }

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            std::string feature_id = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            feature_ids.push_back(feature_id);
        }

        return feature_ids;
    };

    RetCode FileSystemStorage::insert_features_to_meta_db(std::vector<std::string> feature_ids) {
        return RetCode::RET_OK;
    };

} // namespace search
