#pragma once

#include "nlohmann/json.hpp"
#include "search/searcher.h"
#include "types.h"

#include <map>
#include <memory>
#include <sqlite3.h>

using namespace std;

using json = nlohmann::json;

namespace search {

    struct sqlite3_deleter {
        void operator()(sqlite3* db) {
            if (db != nullptr) {
                sqlite3_close_v2(db);
            }
        }
    };

    using unique_sqlite3 = std::unique_ptr<sqlite3, sqlite3_deleter>;

    class FileSystemStorage : public Storage {
      public:
        FileSystemStorage(const json& config);
        ~FileSystemStorage() = default;

        std::vector<std::string> ListFeautreIDs(int start, int limit) override;

        RetCode Init() override;

        std::vector<std::string> AddFeatures(const std::vector<Feature>& features) override;

        std::vector<Feature> LoadFeatures(const std::vector<std::string>& feature_ids) override;

        RetCode RemoveFeatures(const std::vector<std::string>& feature_ids) override;

      private:
        json _config;
        std::filesystem::path _db_dir;
        std::filesystem::path _data_dir;
        std::filesystem::path _meta_dir;

        unique_sqlite3 _meta_db;

        sqlite3* init_features_meta_db(std::string db_filepath);
        std::vector<std::string> list_features_from_meta_db(int start, int limit);
        RetCode insert_features_to_meta_db(std::vector<std::string> feature_ids);

    };

} // namespace search
