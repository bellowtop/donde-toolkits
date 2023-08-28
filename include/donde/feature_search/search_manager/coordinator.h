#pragma once

// #include "spdlog/spdlog.h"
#include "donde/feature_search/definitions.h"
#include "donde/feature_search/search_manager/worker_factory.h"
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
// Coordinator & Reducer, responsible for:
// 1. init shard_manager to manage shards, shards are sharded by date/region/feature-id
// 2. make connection to workers, assign shards to these workers.
// 3. mapping user db to shards.
class ICoordinator {
  public:
    virtual void Start() = 0;

    virtual void Stop() = 0;

    virtual void ProbeWorkers(const WorkerFactory& factory) = 0;

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
class Coordinator : public ICoordinator {
  public:
    Coordinator(const json& coor_config);
    // this dtor declaration is necessary. and its implementation
    // must be placed in .cc file, because we use pimpl with std::unique_ptr
    ~Coordinator();

    void Start() override;

    void Stop() override;

    void ProbeWorkers(const WorkerFactory& factory) override;

    // std::vector<WorkerPtr> ListWorkers() override;

    std::vector<DBItem> ListUserDBs() override;

    // AddFeatures to this db, we need find proper shard to store these fts.
    std::vector<std::string> AddFeatures(const std::string& db_id,
                                         const std::vector<Feature>& fts) override;

    // RemoveFeatures from this db
    RetCode RemoveFeatures(const std::string& db_id,
                           const std::vector<std::string>& feature_ids) override;

    // SearchFeatures in this db.
    std::vector<FeatureSearchItem> SearchFeature(const std::string& db_id, const Feature& query,
                                                 int topk) override;

    std::unique_ptr<CoordinatorImpl> pimpl;
};

} // namespace donde_toolkits::feature_search::search_manager
