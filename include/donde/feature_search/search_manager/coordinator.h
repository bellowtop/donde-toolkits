#pragma once

// #include "spdlog/spdlog.h"
#include "donde/feature_search/definitions.h"
#include "nlohmann/json.hpp"

#include <algorithm>
#include <iostream>
#include <unordered_map>

using namespace std;

using json = nlohmann::json;

namespace donde_toolkits ::feature_search ::search_manager {

class Worker;
using WorkerPtr = shared_ptr<Worker>;

// Coordinator & Reducer
class CoordinatorInterface {
  public:
    virtual void Start() = 0;

    virtual void Stop() = 0;

    // std::vector<WorkerPtr> ListWorkers() = 0;

    virtual std::vector<DBItem> ListUserDBs() = 0;

    // AddFeatures to this db, we need find proper shard to store these fts.
    virtual std::vector<std::string> AddFeatures(const std::string& db_id,
                                                 const std::vector<Feature>& fts)
        = 0;

    // RemoveFeatures from this db
    virtual RetCode RemoveFeatures(const std::string& db_id,
                                   const std::vector<std::string>& feature_ids)
        = 0;

    // SearchFeatures in this db.
    virtual std::vector<FeatureSearchItem> SearchFeature(const std::string& db_id,
                                                         const Feature& query, int topk)
        = 0;
};

class CoordinatorImpl;
class Coordinator : public CoordinatorInterface {
  public:
    Coordinator(const json& coor_config);
    // this dtor declaration is necessary. and its implementation
    // must be placed in .cc file, because we use pimpl with std::unique_ptr
    ~Coordinator();

    void Start();

    void Stop();

    // std::vector<WorkerPtr> ListWorkers();

    std::vector<DBItem> ListUserDBs();

    // AddFeatures to this db, we need find proper shard to store these fts.
    std::vector<std::string> AddFeatures(const std::string& db_id, const std::vector<Feature>& fts);

    // RemoveFeatures from this db
    RetCode RemoveFeatures(const std::string& db_id, const std::vector<std::string>& feature_ids);

    // SearchFeatures in this db.
    std::vector<FeatureSearchItem> SearchFeature(const std::string& db_id, const Feature& query,
                                                 int topk);

    std::unique_ptr<CoordinatorImpl> pimpl;
};

} // namespace donde_toolkits::feature_search::search_manager
