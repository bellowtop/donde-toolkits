#pragma once

#include "SQLiteCpp/SQLiteCpp.h"
#include "nlohmann/json.hpp"
#include "search/searcher.h"
#include "types.h"

#include <map>
#include <memory>

using namespace std;

using json = nlohmann::json;

namespace search {

    inline RetCode init_features_meta_db(SQLite::Database* db) {
        std::string sql = "create table if not exists features("
                          "id integer primary key autoincrement, "
                          "feature_id char(64), "
                          "version int "
                          ");";
        try {
            db->exec(sql);
        } catch (std::exception& exc) {
            std::cerr << "cannot create table: " << exc.what() << std::endl;
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
            std::cerr << "cannot delete from features table: " << exc.what() << std::endl;
            return RetCode::RET_ERR;
        }

        return RetCode::RET_OK;
    }

    inline std::vector<std::string> list_features_from_meta_db(SQLite::Database* db, int start,
                                                               int limit) {
        std::vector<std::string> feature_ids;

        try {
            std::string sql("select * from features limit ? offset ?");
            SQLite::Statement query(*db, sql);
            query.bind(1, limit);
            query.bind(2, start);

            while (query.executeStep()) {
                // int id = query.getColumn(0);
                const char* value = query.getColumn(1);
                std::string feature_id(value);
                feature_ids.push_back(feature_id);
            }
        } catch (std::exception& exc) {
            std::cerr << "cannot select from features table: " << exc.what() << std::endl;
            return feature_ids;
        }

        return feature_ids;
    };

    inline RetCode insert_features_to_meta_db(SQLite::Database* db,
                                              const std::vector<std::string>& feature_ids) {
        // TODO: batch control
        try {
            int version = 10000; // FIXME
            std::string sql("insert into features(feature_id, version) values (?, ?)");
            for (size_t i = 1; i < feature_ids.size(); i++) {
                sql += ", (?, ?)";
            }
            sql += ";";

            SQLite::Statement query(*db, sql);
            for (size_t i = 0; i < feature_ids.size(); i++) {
                query.bind(2 * i + 1, feature_ids[i]);
                query.bind(2 * i + 2, version);
            }

            query.exec();
        } catch (std::exception& exc) {
            std::cerr << "cannot insert into features table: " << exc.what() << std::endl;
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
            std::cerr << "cannot select from features table: " << exc.what() << std::endl;
            return -1;
        }

        return count;
    };

    class FileSystemStorage : public Storage {
      public:
        FileSystemStorage(const json& config);
        ~FileSystemStorage() = default;

        PageData<FeatureIDList> ListFeautreIDs(uint start, uint limit) override;

        RetCode Init() override;

        std::vector<std::string> AddFeatures(const std::vector<Feature>& features) override;

        std::vector<Feature> LoadFeatures(const std::vector<std::string>& feature_ids) override;

        RetCode RemoveFeatures(const std::vector<std::string>& feature_ids) override;

      private:
        json _config;
        std::filesystem::path _db_dir;
        std::filesystem::path _data_dir;
        std::filesystem::path _meta_dir;

        std::unique_ptr<SQLite::Database> _meta_db;
    };

} // namespace search
