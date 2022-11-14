#include "SQLiteCpp/SQLiteCpp.h"
#include "donde/definitions.h"
#include "donde/feature_search/api.h"
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
#include <msgpack.hpp>
#include <nlohmann/json_fwd.hpp>
#include <opencv2/core/hal/interface.h>
#include <sqlite3.h>
#include <sstream>
#include <stdexcept>

using namespace std;

using json = nlohmann::json;

namespace donde {

namespace feature_search {

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// CassandraDriver, use cassandra keyspace to store feature blob.
// ids,
//
//////////////////////////////////////////////////////////////////////////////////////////////////

class CassandraDriver : public Driver {

  public:
    CassandraDriver(std::string addr);

    // the sqlite3 db ptr will auto release when destruct.
    ~CassandraDriver() = default;

    RetCode Init(const std::vector<std::string>& initial_db_ids) override;

    std::string CreateDB(const DBItem& info) override;

    DBItem FindDB(const std::string& db_id) override;

    std::vector<DBItem> ListDBs() override;

    RetCode DeleteDB(std::string db_id) override;

    // Shard management
    std::vector<DBShard> ListShards(const std::string& db_id) override;

    std::string CreateShard(const std::string& db_id, const DBShard& shard) override;

    PageData<FeatureDbItemList> ListFeatures(const std::string& db_id, uint page,
                                             uint perPage) override;

    std::vector<std::string> AddFeatures(const std::string& db_id,
                                         const std::vector<FeatureDbItem>& features) override;

    std::vector<Feature> LoadFeatures(const std::string& db_id,
                                      const std::vector<std::string>& feature_ids) override;

    RetCode RemoveFeatures(const std::string& db_id,
                           const std::vector<std::string>& feature_ids) override;

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

    std::vector<DBItem> list_user_db_items(bool include_deleted = false);

    RetCode update_db_item(const DBItem& new_item);

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

    RetCode delete_features_from_db(const std::string& db_id,
                                    const std::vector<std::string>& feature_ids);

    std::vector<FeatureDbItem> list_features_from_db(const std::string& db_id, int start,
                                                     int limit);

    RetCode insert_features_into_db(const std::string& db_id,
                                    const std::vector<std::string>& feature_ids,
                                    const std::vector<std::string>& metadatas);

    uint64 count_features_in_db(const std::string& db_id);
};

} // namespace feature_search

} // namespace donde
