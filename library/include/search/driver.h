#pragma once

#include "definitions.h"
#include "faiss/Index2Layer.h"
#include "nlohmann/json.hpp"
#include "types.h"

#include <map>
#include <memory>
#include <opencv2/core/hal/interface.h>

using namespace std;

using json = nlohmann::json;

namespace search {

    class Driver {

      public:
        virtual ~Driver() = default;

        virtual RetCode Init(const std::vector<std::string>& initial_db_ids) = 0;

        // DB management
        virtual std::string CreateDB(const DBItem& info) = 0;

        virtual DBItem FindDB(const std::string& db_id) = 0;

        virtual std::vector<DBItem> ListDBs() = 0;

        virtual RetCode DeleteDB(std::string db_id) = 0;

        // Shard Management
        virtual std::vector<DBShard> ListShards(const std::string& db_id) = 0;

        virtual std::string CreateShard(const std::string& db_id, const DBShard& shard) = 0;

        virtual RetCode UpdateShard(const std::string& db_id, const DBShard& shard) = 0;

        virtual std::string CloseShard(const std::string& db_id, const std::string& shard_id) = 0;

        // Feature management
        virtual PageData<FeatureDbItemList> ListFeatures(const std::string& db_id, uint start,
                                                         uint limit)
            = 0;

        virtual std::vector<std::string> AddFeatures(const std::string& db_id,
                                                     const std::vector<FeatureDbItem>& features)
            = 0;

        virtual std::vector<Feature> LoadFeatures(const std::string& db_id,
                                                  const std::vector<std::string>& feature_ids)
            = 0;

        virtual RetCode RemoveFeatures(const std::string& db_id,
                                       const std::vector<std::string>& feature_ids)
            = 0;
    };

} // namespace search
