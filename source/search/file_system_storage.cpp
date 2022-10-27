#include "nlohmann/json.hpp"
#include "search/searcher.h"
#include "search/storage.h"
#include "types.h"
#include "uuid/uuid.h"

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
#include <opencv2/core/hal/interface.h>
#include <sqlite3.h>
#include <sstream>
#include <stdexcept>

using namespace std;

using json = nlohmann::json;

namespace search {

    FileSystemStorage::FileSystemStorage(const json& config)
        : _config(config),
          _db_dir(_config["path"]),
          _data_dir(_db_dir / "data"),
          _meta_dir(_db_dir / "meta") {

        std::filesystem::create_directories(_data_dir);
        std::filesystem::create_directories(_meta_dir);

        std::string db_filepath = _meta_dir / "sqlite3.db";

        try {
            _meta_db = std::make_unique<SQLite::Database>(db_filepath, SQLite::OPEN_READWRITE
                                                                           | SQLite::OPEN_CREATE);
        } catch (std::exception& exc) {
            std::cerr << "cannot open database: " << exc.what() << std::endl;
            std::abort();
        }
    };

    RetCode FileSystemStorage::Init() {
        // init table.
        init_features_meta_db();
        return RetCode::RET_OK;
    };

    PageData<FeatureIDList> FileSystemStorage::ListFeautreIDs(uint page, uint perPage) {
        uint64 count = count_features_in_meta_db();
        uint64 totalPage = count / perPage;
        std::vector<std::string> feature_ids = list_features_from_meta_db(page * perPage, page);

        PageData<FeatureIDList> ret {
            uint64(page),
            uint64(perPage),
            totalPage,
            feature_ids,
        };

        return ret;
    };

    std::vector<std::string> FileSystemStorage::AddFeatures(const std::vector<Feature>& features) {
        int count = features.size();
        std::vector<std::string> feature_ids;
        for (auto& ft : features) {
            std::string feature_id;
            feature_id.resize(32);

            uuid_t uuid;
            uuid_generate(uuid);
            uuid_unparse_lower(uuid, feature_id.data());

            std::cout << "uuid: " << feature_id << std::endl;

            auto filepath = _data_dir / (feature_id + ".ft");
            try {
                std::ofstream file(filepath, std::ios::binary | std::ios::out);
                std::stringstream ss;
                msgpack::pack(ss, ft);

                // write to file
                std::string data(ss.str());
                file << data;

                // std::cout << "write data.size" << data.size() << std::endl;
                feature_ids.push_back(feature_id);

            } catch (const std::exception& exc) {
                std::cerr << "cannot save feature to " << filepath << ", exc: " << exc.what()
                          << std::endl;
                feature_ids.push_back("");
            }
        }

        // insert feature to meta db.
        insert_features_to_meta_db(feature_ids);

        return feature_ids;
    };

    std::vector<Feature>
    FileSystemStorage::LoadFeatures(const std::vector<std::string>& feature_ids) {
        int count = feature_ids.size();
        std::vector<Feature> features;
        for (auto& feature_id : feature_ids) {
            std::cout << "load feature_id: " << feature_id << std::endl;

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
                std::cerr << "cannot load feature, feature_path: " << filepath << exc.what()
                          << std::endl;
                features.push_back(Feature{});
            }
        }

        return features;
    };

    RetCode FileSystemStorage::RemoveFeatures(const std::vector<std::string>& feature_ids) {
        if (feature_ids.size() == 0) {
            return RetCode::RET_OK;
        }

        for (auto& feature_id : feature_ids) {
            auto filepath = _data_dir / (feature_id + ".ft");
            std::filesystem::remove(filepath);
        }

        delete_features_from_meta_db(feature_ids);

        return RetCode::RET_OK;
    };

    RetCode FileSystemStorage::init_features_meta_db() {
        std::string sql = "create table if not exists features("
                          "id integer primary key autoincrement, "
                          "feature_id char(64), "
                          "version int "
                          ");";
        try {
            _meta_db->exec(sql);
        } catch (std::exception& exc) {
            std::cerr << "cannot create table: " << exc.what() << std::endl;
            std::abort();
        }

        return RetCode::RET_OK;
    };

    RetCode FileSystemStorage::delete_features_from_meta_db(const std::vector<std::string>& feature_ids) {
        try {
            std::string sql = "delete from features where feature_id in (? ";
            for (int i = 1; i < feature_ids.size(); i ++) {
                sql += ",? ";
            }
            sql += ")";

            SQLite::Statement query(*(_meta_db.get()), sql);

            for (int i = 0; i < feature_ids.size(); i ++) {
                query.bind(i + 1, feature_ids[i]);
            }

            query.exec();
        } catch (std::exception& exc) {
            std::cerr << "cannot delete from features table: " << exc.what() << std::endl;
        }

        return RetCode::RET_OK;
    }

    std::vector<std::string> FileSystemStorage::list_features_from_meta_db(int start, int limit) {
        std::vector<std::string> feature_ids;

        try {
            std::string sql("select * from features limit ? offset ?");
            SQLite::Statement query(*(_meta_db.get()), sql);
            query.bind(1, limit);
            query.bind(2, start);

            while (query.executeStep()) {
                int id = query.getColumn(0);
                const char* value = query.getColumn(1);
                std::string feature_id(value);
                feature_ids.push_back(feature_id);
            }
        } catch (std::exception& exc) {
            std::cerr << "cannot select from features table: " << exc.what() << std::endl;
        }

        return feature_ids;
    };

    RetCode FileSystemStorage::insert_features_to_meta_db(const std::vector<std::string> & feature_ids) {
        // TODO: batch control
        try {
            int version = 10000; // FIXME
            std::string sql("insert into features(feature_id, version) values (?, ?)");
            for (int i = 1; i < feature_ids.size(); i++) {
                sql += ", (?, ?)";
            }
            sql += ";";

            SQLite::Statement query(*(_meta_db.get()), sql);
            for (int i = 0; i < feature_ids.size(); i++) {
                query.bind(2*i + 1, feature_ids[i]);
                query.bind(2*i + 2, version);
            }

            query.exec();
        } catch (std::exception& exc) {
            std::cerr << "cannot insert into features table: " << exc.what() << std::endl;
        }

        return RetCode::RET_OK;
    };

    uint64 FileSystemStorage::count_features_in_meta_db() {

        return 0;
    };


} // namespace search
