#pragma once

#include "Poco/Format.h"
#include "SQLiteCpp/SQLiteCpp.h"
#include "nlohmann/json.hpp"
#include "search/db_searcher.h"
#include "search/driver.h"
#include "utils.h"

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
#include <nlohmann/json_fwd.hpp>
#include <opencv2/core/hal/interface.h>
#include <sqlite3.h>
#include <sstream>
#include <stdexcept>

using namespace std;

using json = nlohmann::json;

using Poco::format;

namespace search {

    //////////////////////////////////////////////////////////////////////////////////////////////////
    //
    // SimpleDriver, use filesystem to store feature files, use sqlite3 to store feature ids,
    //
    //////////////////////////////////////////////////////////////////////////////////////////////////

    class SimpleDriver : public Driver {

      public:
        SimpleDriver(std::string db_dirpath)
            : _db_dir(db_dirpath), _data_dir(_db_dir / "data"), _meta_dir(_db_dir / "meta") {

            std::filesystem::create_directories(_data_dir);
            std::filesystem::create_directories(_meta_dir);

            std::string db_filepath = _meta_dir / "sqlite3.db";

            try {
                db = std::make_unique<SQLite::Database>(db_filepath, SQLite::OPEN_READWRITE
                                                                         | SQLite::OPEN_CREATE);
            } catch (std::exception& exc) {
                spdlog::error("cannot open database: ", exc.what());
                std::abort();
            }
        };

        // the sqlite3 db ptr will auto release when destruct.
        ~SimpleDriver() = default;

        std::string CreateDB(DBItem& info) { return {}; };

        DBItem FindDB(const std::string& db_id) { return {}; };

        std::vector<DBItem> ListDBs() { return {}; };

        RetCode DeleteDB(std::string db_id) { return RetCode::RET_OK; };

        PageData<FeatureDbItemList> ListFeatures(const std::string& db_id, uint page,
                                                 uint perPage) {
            uint64 count = count_features_in_meta_db(db_id);
            if (count <= 0) {
                spdlog::warn("FileSystemStorage::ListFeatures, count is {}", count);
                return {};
            }

            uint64 totalPage = (count + perPage - 1) / perPage;

            std::vector<FeatureDbItem> feature_ids
                = list_features_from_meta_db(db_id, page * perPage, perPage);
            spdlog::debug("feature_ids size: ", feature_ids.size());

            PageData<FeatureDbItemList> ret{uint64(page), uint64(perPage), totalPage, feature_ids};
            return ret;
        };

        RetCode Init(const std::vector<std::string>& initial_db_ids) {
            // init tables.
            for (const auto& id : initial_db_ids) {
                init_features_meta_db(id);
            }
            return RetCode::RET_OK;
        };

        std::vector<std::string> AddFeatures(const std::string& db_id,
                                             const std::vector<FeatureDbItem>& features) {
            int count = features.size();
            std::vector<std::string> feature_ids;
            std::vector<std::string> metadatas;

            for (auto& item : features) {
                auto ft = item.feature;

                std::string feature_id = generate_uuid();
                // std::cout << "generated feature_id: " << feature_id << std::endl;

                auto filepath = _data_dir / (feature_id + ".ft");
                try {
                    std::ofstream file(filepath, std::ios::binary | std::ios::out);
                    std::stringstream ss;
                    msgpack::pack(ss, ft);

                    // write to file
                    std::string data(ss.str());
                    file << data;

                    json j(item.metadata); // must use (), while not {}
                    std::string meta_str{j.dump()};
                    metadatas.push_back(meta_str);

                    // std::cout << "write data.size" << data.size() << std::endl;
                    feature_ids.push_back(feature_id);

                } catch (const std::exception& exc) {
                    spdlog::error("cannot save feature to : {}, exc: {}", filepath.string(),
                                  exc.what());
                    feature_ids.push_back("");
                    metadatas.push_back("{}");
                }
            }

            // insert feature to meta db.
            insert_features_to_meta_db(db_id, feature_ids, metadatas);

            return feature_ids;
        };

        std::vector<Feature> LoadFeatures(const std::string& db_id,
                                          const std::vector<std::string>& feature_ids) {
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
                    spdlog::error("cannot load feature, feature_path: {}, exc: {}",
                                  filepath.string(), exc.what());
                    features.push_back({});
                }
            }

            return features;
        };

        RetCode RemoveFeatures(const std::string& db_id,
                               const std::vector<std::string>& feature_ids) {
            if (feature_ids.size() == 0) {
                return RetCode::RET_OK;
            }

            for (auto& feature_id : feature_ids) {
                auto filepath = _data_dir / (feature_id + ".ft");
                std::filesystem::remove(filepath);
            }

            return delete_features_from_meta_db(db_id, feature_ids);
        };

      private:
        std::filesystem::path _db_dir;
        std::filesystem::path _data_dir;
        std::filesystem::path _meta_dir;

        std::unique_ptr<SQLite::Database> db;

        //
        // SQLite3 operations.
        //
      private:
        RetCode init_features_meta_db(const std::string& db_id) {
            std::string sql = "create table if not exists features_db_%s("
                              "id integer primary key autoincrement, "
                              "feature_id char(64), "
                              "metadata text, "
                              "version int "
                              ");";
            sql = format(sql, db_id);
            try {
                db->exec(sql);
            } catch (std::exception& exc) {
                spdlog::error("cannot create table: {}", exc.what());
                return RetCode::RET_ERR;
            }

            return RetCode::RET_OK;
        };

        RetCode delete_features_from_meta_db(const std::string& db_id,
                                             const std::vector<std::string>& feature_ids) {
            try {
                std::string sql = "delete from features_db_%s where feature_id in (? ";
                sql = format(sql, db_id);

                for (size_t i = 1; i < feature_ids.size(); i++) {
                    sql += ",? ";
                }
                sql += ")";

                SQLite::Statement query(*db, sql);

                for (size_t i = 0; i < feature_ids.size(); i++) {
                    query.bind(i + 1, feature_ids[i]);
                }

                query.exec();
            } catch (std::exception& exc) {
                spdlog::error("cannot delete from features table: {}", exc.what());
                return RetCode::RET_ERR;
            }

            return RetCode::RET_OK;
        }

        std::vector<FeatureDbItem> list_features_from_meta_db(const std::string& db_id, int start,
                                                              int limit) {
            std::vector<FeatureDbItem> feature_ids;

            try {
                std::string sql("select feature_id, metadata from features limit ? offset ?");
                SQLite::Statement query(*db, sql);
                query.bind(1, limit);
                query.bind(2, start);

                while (query.executeStep()) {
                    // int id = query.getColumn(0);
                    std::string feature_id = query.getColumn(0).getString();
                    std::string meta_str = query.getColumn(1).getText();
                    // std::cout << "feature_id: " << feature_id << std::endl;
                    // std::cout << "meta_str: " << meta_str << std::endl;

                    json j(json::parse(meta_str));

                    std::map<string, string> meta_map = j;

                    feature_ids.push_back(FeatureDbItem{
                        .feature_id = feature_id,
                        .metadata = meta_map,
                    });
                }
            } catch (std::exception& exc) {
                spdlog::error("cannot select from features table: {}", exc.what());
                return feature_ids;
            }

            return feature_ids;
        };

        RetCode insert_features_to_meta_db(const std::string& db_id,
                                           const std::vector<std::string>& feature_ids,
                                           const std::vector<std::string>& metadatas) {
            // TODO: batch control
            try {
                int version = 10000; // FIXME
                std::string sql(
                    "insert into features(feature_id, metadata, version) values (?, ?, ?)");
                for (size_t i = 1; i < feature_ids.size(); i++) {
                    sql += ", (?, ?, ?)";
                }
                sql += ";";

                SQLite::Statement query(*db, sql);
                for (size_t i = 0; i < feature_ids.size(); i++) {
                    auto feature_id = feature_ids[i];
                    auto metadata = metadatas[i];

                    query.bind(3 * i + 1, feature_id);
                    query.bind(3 * i + 2, metadata);
                    query.bind(3 * i + 3, version);
                }

                query.exec();
            } catch (std::exception& exc) {
                spdlog::error("cannot insert into features table: {}", exc.what());
                return RetCode::RET_ERR;
            }

            return RetCode::RET_OK;
        };

        uint64 count_features_in_meta_db(const std::string& db_id) {
            int count;

            try {
                std::string sql("select count(*) from features;");
                SQLite::Statement query(*db, sql);

                query.executeStep();
                count = query.getColumn(0);
            } catch (std::exception& exc) {
                spdlog::error("cannot count from features table: {}", exc.what());
                return -1;
            }

            return count;
        };
    };

} // namespace search
