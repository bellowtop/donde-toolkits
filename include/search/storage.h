#pragma once

#include "nlohmann/json.hpp"
#include "search/searcher.h"
#include "types.h"

#include <map>

using namespace std;

using json = nlohmann::json;

namespace search {

    class FileSystemStorage : public Storage {
      public:
        FileSystemStorage(const json& config);
        ~FileSystemStorage() = default;

        std::vector<std::string> AddFeatures(const std::vector<Feature>& features) override;

        RetCode RemoveFeatures(const std::vector<std::string>& feature_ids) override;

      private:
        json _config;
        std::filesystem::path _db_dir;
        std::filesystem::path _data_dir;
        std::filesystem::path _meta_dir;
    };

} // namespace search
