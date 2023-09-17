#include "donde/definitions.h"
#include "donde/feature_search/definitions.h"
#include "donde/feature_search/driver.h"
#include "donde/utils.h"
#include "fmt/format.h"
#include "nlohmann/json.hpp"

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
//// #include <msgpack.hpp>
#include <nlohmann/json_fwd.hpp>
#include <opencv2/core/hal/interface.h>
#include <sqlite3.h>
#include <sstream>
#include <stdexcept>

using namespace std;

using json = nlohmann::json;

namespace donde_toolkits ::feature_search {

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// SimpleDriver, use filesystem to store feature files, use sqlite3 to store feature ids,
//
//////////////////////////////////////////////////////////////////////////////////////////////////

class SimpleDriver : public Driver {

  public:
    SimpleDriver(std::string db_dirpath);

    // the sqlite3 db ptr will auto release when destruct.
    ~SimpleDriver() = default;

    RetCode Init(const std::vector<std::string>& initial_db_ids) override;

    // DB management
    std::string CreateDB(const DBItem& info) override;

    DBItem FindDB(const std::string& db_id) override;

    std::vector<DBItem> ListDBs() override;

    RetCode DeleteDB(std::string db_id) override;

    // Worker management
    std::vector<WorkerItem> ListWorkers() override;

    void CreateWorker(const std::string& worker_id, const WorkerItem& worker) override;

    void UpdateWorker(const std::string& worker_id, const WorkerItem& worker) override;

    // Shard management
    std::vector<DBShard> ListShards(const std::string& db_id) override;

    std::string CreateShard(const std::string& db_id, const DBShard& shard) override;

    RetCode UpdateShard(const std::string& db_id, const DBShard& shard) override;

    std::string CloseShard(const std::string& db_id, const std::string& shard) override;

    PageData<FeatureDbItemList> ListFeatures(uint page, uint perPage, const std::string& db_id,
                                             const std::string& shard_id = "") override;

    std::vector<std::string> AddFeatures(const std::vector<FeatureDbItem>& features,
                                         const std::string& db_id,
                                         const std::string& shard_id) override;

    std::vector<Feature> LoadFeatures(const std::vector<std::string>& feature_ids,
                                      const std::string& db_id,
                                      const std::string& shard_id) override;

    RetCode RemoveFeatures(const std::vector<std::string>& feature_ids, const std::string& db_id,
                           const std::string& shard_id) override;

  private:
    std::filesystem::path _db_dir;
    std::filesystem::path _data_dir;
    std::filesystem::path _meta_dir;

    std::unique_ptr<SQLite::Database> db;

    std::vector<DBItem> _cached_db_items;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // SQLite3 operations.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
  private:
    ///
    /// DB management
    ///
    RetCode init_user_db_table();

    RetCode insert_into_user_dbs(const std::string& db_id, const std::string& name, uint64 capacity,
                                 const std::string& desc);

    std::vector<DBItem> list_user_dbs(bool include_deleted = false);

    RetCode update_user_db(const DBItem& new_item);

    RetCode delete_user_db(const std::string& db_id);

    ///
    /// Shards management
    ///

    RetCode insert_into_db_shards(const std::string& db_id, const std::string& shard_id,
                                  uint64 capacity);

    std::vector<DBShard> list_db_shards(const std::string& db_id);

    ///
    /// Features management
    ///

    RetCode init_features_table_for_db(const std::string& db_id);

    RetCode insert_features_into_db(const std::vector<std::string>& feature_ids,
                                    const std::vector<std::string>& metadatas,
                                    const std::string& db_id, const std::string& shard_id);

    RetCode delete_features_from_db(const std::vector<std::string>& feature_ids,
                                    const std::string& db_id, const std::string& shard_id = "");

    std::vector<FeatureDbItem> list_features_from_db(int start, int limit, const std::string& db_id,
                                                     const std::string& shard_id = "");

    uint64 count_features_in_db(const std::string& db_id, const std::string& shard_id = "");

    static inline std::string feature_table_name(const std::string& db_id) {
        return "fetures_db_" + replace_underscore_for_uuid(db_id);
    };
};

} // namespace donde_toolkits::feature_search
