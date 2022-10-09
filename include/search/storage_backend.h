#pragma once

#include "nlohmann/json.hpp"
#include "search/searcher.h"
#include "types.h"

#include <map>

using namespace std;

using json = nlohmann::json;

namespace search {

    class FileSystemBackend : public StorageBackend {
      public:
        FileSystemBackend(const json& config);
        ~FileSystemBackend() = default;

        std::vector<uint64> AddFeatures(const std::vector<Feature>& features) override;

        RetCode RemoveFeatures(const std::vector<uint64>& feature_ids) override;

      private:
        json _config;
    };

} // namespace search
