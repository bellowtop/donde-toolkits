#include "simple_driver.h"

#include "SQLiteCpp/SQLiteCpp.h"
#include "fmt/format.h"
#include "nlohmann/json.hpp"

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>
#include <chrono>
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
#include <typeinfo>

using namespace std;

using json = nlohmann::json;

using fmt::format;

namespace donde_toolkits {

namespace feature_search {

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// SimpleDriver, use filesystem to store feature files, use sqlite3 to store feature ids,
//
//////////////////////////////////////////////////////////////////////////////////////////////////

SimpleDriver::SimpleDriver(std::string db_dirpath)
    : _db_dir(db_dirpath), _data_dir(_db_dir / "data"), _meta_dir(_db_dir / "meta") {

    std::filesystem::create_directories(_data_dir);
    std::filesystem::create_directories(_meta_dir);

    std::string db_filepath = _meta_dir / "sqlite3.db";

    try {
        db = std::make_unique<SQLite::Database>(db_filepath,
                                                SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

        init_user_db_table();
    } catch (std::exception& exc) {
        spdlog::error("cannot open database: ", exc.what());
        std::abort();
    }
};

std::string SimpleDriver::CreateDB(const DBItem& info) {
    // expire cache.
    _cached_db_items = {};

    std::string db_id = generate_uuid();
    insert_into_user_dbs(db_id, info.name, info.capacity, info.description);
    init_features_table_for_db(db_id);

    return db_id;
};

DBItem SimpleDriver::FindDB(const std::string& db_id) {
    if (_cached_db_items.size() == 0) {
        _cached_db_items = list_user_dbs();
    }
    for (auto& v : _cached_db_items) {
        if (v.db_id == db_id) {
            return v;
        }
    }
    return {};
};

std::vector<DBItem> SimpleDriver::ListDBs() {
    if (_cached_db_items.size() == 0) {
        _cached_db_items = list_user_dbs();
    }
    return _cached_db_items;
};

RetCode SimpleDriver::DeleteDB(std::string db_id) {
    // expire cache.
    _cached_db_items = {};

    delete_user_db(db_id);

    return RetCode::RET_OK;
};

std::vector<DBShard> SimpleDriver::ListShards(const std::string& db_id) { return {}; };

std::string SimpleDriver::CreateShard(const std::string& db_id, const DBShard& shard) {
    return {};
};

RetCode SimpleDriver::UpdateShard(const std::string& db_id, const DBShard& shard) { return {}; };

std::string SimpleDriver::CloseShard(const std::string& db_id, const std::string& shard) {
    return {};
};

PageData<FeatureDbItemList> SimpleDriver::ListFeatures(uint page, uint perPage,
                                                       const std::string& db_id,
                                                       const std::string& shard_id) {
    uint64 count = count_features_in_db(db_id);
    if (count <= 0) {
        spdlog::warn("FileSystemStorage::ListFeatures, count is {}", count);
        return {};
    }

    uint64 totalPage = (count + perPage - 1) / perPage;

    std::vector<FeatureDbItem> feature_ids
        = list_features_from_db(page * perPage, perPage, db_id, shard_id);
    spdlog::debug("feature_ids size: ", feature_ids.size());

    PageData<FeatureDbItemList> ret{uint64(page), uint64(perPage), totalPage, feature_ids};
    return ret;
};

RetCode SimpleDriver::Init(const std::vector<std::string>& initial_db_ids) {
    // init tables.
    for (const auto& id : initial_db_ids) {
        init_features_table_for_db(id);
    }
    return RetCode::RET_OK;
};

std::vector<std::string> SimpleDriver::AddFeatures(const std::vector<FeatureDbItem>& features,
                                                   const std::string& db_id,
                                                   const std::string& shard_id) {
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
            spdlog::error("cannot save feature to : {}, exc: {}", filepath.string(), exc.what());
            feature_ids.push_back("");
            metadatas.push_back("{}");
        }
    }

    // insert feature to meta db.
    insert_features_into_db(feature_ids, metadatas, db_id, shard_id);

    return feature_ids;
};

std::vector<Feature> SimpleDriver::LoadFeatures(const std::vector<std::string>& feature_ids,
                                                const std::string& db_id,
                                                const std::string& shard_id) {
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
            spdlog::error("cannot load feature, feature_path: {}, exc: {}", filepath.string(),
                          exc.what());
            features.push_back({});
        }
    }

    return features;
};

RetCode SimpleDriver::RemoveFeatures(const std::vector<std::string>& feature_ids,
                                     const std::string& db_id, const std::string& shard_id) {
    if (feature_ids.size() == 0) {
        return RetCode::RET_OK;
    }

    for (auto& feature_id : feature_ids) {
        auto filepath = _data_dir / (feature_id + ".ft");
        std::filesystem::remove(filepath);
    }

    return delete_features_from_db(feature_ids, db_id, shard_id);
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////
// SQLite3 operations.
//////////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// DB management
///
RetCode SimpleDriver::init_user_db_table() {
    std::string sql = "create table if not exists dbs("
                      "id integer primary key autoincrement, "
                      "db_id char(64), "
                      "name char(64), "
                      "capacity integer, "
                      "description text, "
                      "is_deleted boolean, "
                      "created_at datetime, "
                      "updated_at datetime, "
                      "deleted_at datetime "
                      ");";
    try {
        db->exec(sql);
    } catch (std::exception& exc) {
        spdlog::error("cannot create table dbs, exc: {}", exc.what());
        return RetCode::RET_ERR;
    }

    std::string sql2 = "create table if not exists db_shards("
                       "id integer primary key autoincrement, "
                       "shard_id char(64), "
                       "db_id char(64), "
                       "capacity integer, "
                       "is_closed boolean, "
                       "created_at datetime, "
                       "updated_at datetime, "
                       "deleted_at datetime "
                       ");";
    try {
        db->exec(sql2);
    } catch (std::exception& exc) {
        spdlog::error("cannot create table dbs, exc: {}", exc.what());
        return RetCode::RET_ERR;
    }

    return RetCode::RET_OK;
};

RetCode SimpleDriver::insert_into_user_dbs(const std::string& db_id, const std::string& name,
                                           uint64 capacity, const std::string& desc) {
    try {
        std::string sql("insert into dbs(db_id, name, capacity, description, is_deleted, "
                        "created_at) values (?, ?, ?, ?, ?, ?);");

        time_t now = time(nullptr);

        SQLite::Statement query(*db, sql);

        query.bind(1, db_id);
        query.bind(2, name);
        query.bind(3, int64(capacity));
        query.bind(4, desc);

        query.bind(5, false);
        query.bind(6, int64(now));

        query.exec();
    } catch (std::exception& exc) {
        spdlog::error("cannot insert into dbs table, exc: {}", exc.what());
        return RetCode::RET_ERR;
    }

    return RetCode::RET_OK;
};

std::vector<DBItem> SimpleDriver::list_user_dbs(bool include_deleted) {
    std::vector<DBItem> dbs;

    try {
        std::string sql("select db_id, name, capacity, description from dbs ");
        if (!include_deleted) {
            sql += "where is_deleted = false";
        }
        sql += ";";

        SQLite::Statement query(*db, sql);
        while (query.executeStep()) {
            std::string db_id = query.getColumn(0).getString();
            std::string name = query.getColumn(1).getText();
            // type convert, int64 => uint64, because we are sure that capacity will not
            // exceed int64, so that's fine
            uint64 capacity = query.getColumn(2).getInt64();
            std::string description = query.getColumn(3).getText();

            dbs.push_back(DBItem{
                .db_id = db_id,
                .name = name,
                .capacity = capacity,
                .description = description,
            });
        }
    } catch (std::exception& exc) {
        spdlog::error("cannot select from dbs table: {}", exc.what());
        return dbs;
    }

    return dbs;
};

RetCode SimpleDriver::update_user_db(const DBItem& new_item) {
    try {
        std::string sql(
            "update dbs set name=?, capacity=?, used=?, description=? where db_id = ?;");

        SQLite::Statement query(*db, sql);
        query.bind(1, new_item.name);
        query.bind(2, int64(new_item.capacity));
        query.bind(3, int64(new_item.used));
        query.bind(4, new_item.description);

        query.exec();

    } catch (std::exception& exc) {
        spdlog::error("cannot update dbs table, exc: {}", exc.what());
        return RetCode::RET_ERR;
    }

    return RetCode::RET_OK;
};

RetCode SimpleDriver::delete_user_db(const std::string& db_id) {
    try {
        std::string sql("update dbs set is_deleted=?, deleted_at=? where db_id = ?;");

        SQLite::Statement query(*db, sql);

        time_t now = time(nullptr); // seconds since 1970
        query.bind(1, true);
        query.bind(2, int64(now));
        query.bind(3, db_id);

        query.exec();

    } catch (std::exception& exc) {
        spdlog::error("cannot update dbs table, exc: {}", exc.what());
        return RetCode::RET_ERR;
    }

    return RetCode::RET_OK;
};

RetCode SimpleDriver::insert_into_db_shards(const std::string& db_id, const std::string& shard_id,
                                            uint64 capacity) {
    try {
        std::string sql("insert into db_shards(db_id, shard_id, capacity, is_closed, "
                        "created_at) values (?, ?, ?, ?, ?);");

        time_t now = time(nullptr);

        SQLite::Statement query(*db, sql);

        query.bind(1, db_id);
        query.bind(2, shard_id);
        query.bind(3, int64(capacity));

        // sqlite3 treat boolean as int, so 0 is false.
        query.bind(4, 0);
        query.bind(5, int64(now));

        query.exec();
    } catch (std::exception& exc) {
        spdlog::error("cannot insert into dbs table, exc: {}", exc.what());
        return RetCode::RET_ERR;
    }

    return RetCode::RET_OK;
};

std::vector<DBShard> SimpleDriver::list_db_shards(const std::string& db_id) {
    std::vector<DBShard> shards;

    try {
        std::string sql("select db_id, shard_id, capacity, is_closed from db_shards;");

        SQLite::Statement query(*db, sql);
        while (query.executeStep()) {
            std::string db_id = query.getColumn(0).getString();
            std::string shard_id = query.getColumn(1).getText();
            // type convert, int64 => uint64, because we are sure that capacity will not
            // exceed int64, so that's fine
            uint64 capacity = query.getColumn(2).getInt64();
            // sqlite3 treat boolean as int
            bool is_closed = query.getColumn(3).getInt() == 1;

            shards.push_back(DBShard{
                .db_id = db_id,
                .shard_id = shard_id,
                .capacity = capacity,
                .is_closed = is_closed,
            });
        }
    } catch (std::exception& exc) {
        spdlog::error("cannot select from dbs table: {}", exc.what());
    }

    return shards;
}

///
/// Features management
///

RetCode SimpleDriver::init_features_table_for_db(const std::string& db_id) {
    std::stringstream ss;
    ss << "create table if not exists " << feature_table_name(db_id);
    ss << "("
          "  id integer primary key autoincrement, "
          "  feature_id char(64), "
          "  shard_id char(64), "
          "  metadata text, "
          "  version int "
          ");";

    try {
        db->exec(ss.str());
    } catch (std::exception& exc) {
        spdlog::error("cannot create table features_db_{}, exc: {}", db_id, exc.what());
        return RetCode::RET_ERR;
    }

    return RetCode::RET_OK;
};

RetCode SimpleDriver::delete_features_from_db(const std::vector<std::string>& feature_ids,
                                              const std::string& db_id,
                                              const std::string& shard_id) {
    try {
        std::stringstream ss;
        ss << "delete from " << feature_table_name(db_id);
        ss << " where feature_id in (? ";
        for (size_t i = 1; i < feature_ids.size(); i++) {
            ss << ",? ";
        }
        ss << ")";

        std::string sql = ss.str();

        SQLite::Statement query(*db, sql);

        for (size_t i = 0; i < feature_ids.size(); i++) {
            query.bind(i + 1, feature_ids[i]);
        }

        query.exec();
    } catch (std::exception& exc) {
        spdlog::error("cannot delete from features table {}: {}", feature_table_name(db_id),
                      exc.what());
        return RetCode::RET_ERR;
    }

    return RetCode::RET_OK;
}

RetCode SimpleDriver::insert_features_into_db(const std::vector<std::string>& feature_ids,
                                              const std::vector<std::string>& metadatas,
                                              const std::string& db_id,
                                              const std::string& shard_id) {
    // TODO: batch control
    try {
        int version = 10000; // FIXME

        std::stringstream ss;
        ss << "insert into " << feature_table_name(db_id);
        ss << "(feature_id, shard_id, metadata, version) values (?, ?, ?, ?) ";

        for (size_t i = 1; i < feature_ids.size(); i++) {
            ss << ", (?, ?, ?, ?)";
        }
        ss << ";";

        std::string sql = ss.str();

        SQLite::Statement query(*db, sql);
        for (size_t i = 0; i < feature_ids.size(); i++) {
            auto feature_id = feature_ids[i];
            auto metadata = metadatas[i];

            query.bind(4 * i + 1, feature_id);
            query.bind(4 * i + 2, shard_id);
            query.bind(4 * i + 3, metadata);
            query.bind(4 * i + 4, version);
        }

        query.exec();
    } catch (std::exception& exc) {
        spdlog::error("cannot insert into features table {}: {}", feature_table_name(db_id),
                      exc.what());
        return RetCode::RET_ERR;
    }

    return RetCode::RET_OK;
};

std::vector<FeatureDbItem> SimpleDriver::list_features_from_db(int start, int limit,
                                                               const std::string& db_id,
                                                               const std::string& shard_id) {
    std::vector<FeatureDbItem> feature_ids;

    try {
        std::stringstream ss;
        ss << "select feature_id, metadata from " << feature_table_name(db_id);
        if (shard_id.size() > 0) {
            ss << " where shard_id = ? limit ? offset ? ;";
        } else {
            ss << "                    limit ? offset ? ;";
        }

        std::string sql = ss.str();
        spdlog::info("sql: {}", sql);

        SQLite::Statement query(*db, sql);
        if (shard_id.size() > 0) {
            query.bind(1, shard_id);
            query.bind(2, limit);
            query.bind(3, start);
        } else {
            query.bind(1, limit);
            query.bind(2, start);
        }

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
        spdlog::error("cannot select from features table {}: {}", feature_table_name(db_id),
                      exc.what());
        return feature_ids;
    }

    return feature_ids;
};

uint64 SimpleDriver::count_features_in_db(const std::string& db_id, const std::string& shard_id) {
    int count;

    try {
        std::stringstream ss;
        ss << "select count(*) from " << feature_table_name(db_id);

        if (shard_id.size() > 0) {
            ss << " where shard_id = ? ;";
        } else {
            ss << "                    ;";
        }

        std::string sql = ss.str();
        spdlog::info("sql: {}", sql);

        SQLite::Statement query(*db, sql);
        if (shard_id.size() > 0) {
            query.bind(1, shard_id);
        }

        query.executeStep();
        count = query.getColumn(0);
    } catch (std::exception& exc) {
        spdlog::error("cannot count from features table {}: {}", feature_table_name(db_id),
                      exc.what());
        return -1;
    }

    return count;
};

} // namespace feature_search

} // namespace donde_toolkits
