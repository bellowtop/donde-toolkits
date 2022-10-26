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

        std::unique_ptr<SQLite::Database> _meta_db;

        RetCode init_features_meta_db();
        std::vector<std::string> list_features_from_meta_db(int start, int limit);
        RetCode insert_features_to_meta_db(std::vector<std::string> feature_ids);
    };

} // namespace search
