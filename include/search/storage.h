#pragma once

#include "SQLiteCpp/SQLiteCpp.h"
#include "nlohmann/json.hpp"
#include "search/searcher.h"
#include "types.h"

#include <map>
#include <memory>
#include <nlohmann/json_fwd.hpp>

using namespace std;

using json = nlohmann::json;

namespace search {

    inline RetCode init_features_meta_db(SQLite::Database* db) {
        std::string sql = "create table if not exists features("
                          "id integer primary key autoincrement, "
                          "feature_id char(64), "
                          "metadata text, "
                          "version int "
                          ");";
        try {
            db->exec(sql);
        } catch (std::exception& exc) {
            spdlog::error("cannot create table: {}", exc.what());
            return RetCode::RET_ERR;
        }

        return RetCode::RET_OK;
    };

    inline RetCode delete_features_from_meta_db(SQLite::Database* db,
                                                const std::vector<std::string>& feature_ids) {
        try {
            std::string sql = "delete from features where feature_id in (? ";
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

    inline std::vector<FeatureDbItem> list_features_from_meta_db(SQLite::Database* db, int start,
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

    inline RetCode insert_features_to_meta_db(SQLite::Database* db,
                                              const std::vector<std::string>& feature_ids,
                                              const std::vector<std::string>& metadatas) {
        // TODO: batch control
        try {
            int version = 10000; // FIXME
            std::string sql("insert into features(feature_id, metadata, version) values (?, ?, ?)");
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

    inline uint64 count_features_in_meta_db(SQLite::Database* db) {
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

    class FileSystemStorage : public Storage {
      public:
        FileSystemStorage(const json& config);
        ~FileSystemStorage() = default;

        PageData<FeatureDbItemList> ListFeatures(uint start, uint limit) override;

        RetCode Init() override;

        std::vector<std::string> AddFeatures(const std::vector<FeatureDbItem>& features) override;

        std::vector<Feature>
        LoadFeatures(const std::vector<std::string>& feature_ids) override;

        RetCode RemoveFeatures(const std::vector<std::string>& feature_ids) override;

      private:
        json _config;
        std::filesystem::path _db_dir;
        std::filesystem::path _data_dir;
        std::filesystem::path _meta_dir;

        std::unique_ptr<SQLite::Database> _meta_db;
    };

} // namespace search
