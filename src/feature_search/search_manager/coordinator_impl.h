#pragma once

#include "donde/feature_search/driver.h"
#include "donde/feature_search/search_manager/coordinator.h"
#include "donde/feature_search/search_manager/worker_factory.h"
#include "donde/feature_search/search_manager/worker_manager.h"
#include "donde/feature_search/shard.h"
#include "donde/feature_search/worker.h"
#include "shard_factory.h"

// #include "spdlog/spdlog.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <iostream>
#include <thread>
#include <unordered_map>



namespace donde_toolkits ::feature_search ::search_manager {

using WorkerPtr = std::shared_ptr<Worker>;

// Coordinator & Reducer
class CoordinatorImpl : public ICoordinator {
  public:
    CoordinatorImpl(const json& coor_config);
    ~CoordinatorImpl();

    void Start() override;

    void Stop() override;

    // ProbeWorkers is a background task, probe each known workers,
    // check if they are up and alive, if one is alive, then factory should create
    // a `worker` impl to interact with it.
    // User must call this method to build connections with known  workers.
    void ProbeWorkers(const WorkerFactory& factory) override;

    // std::vector<WorkerPtr> ListWorkers();

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

  private:
    void initialize_workers();
    void deinitialize_workers();
    void load_known_shards();

    void load_user_dbs();

  private:
    const json& config;

    std::vector<DBItem> _db_items;

    std::vector<std::string> _worker_addrs;
    std::vector<std::string> _invalid_worker_addrs;

    std::unordered_map<std::string, std::string> _managed_workers;

    std::shared_ptr<Driver> _driver;

    std::shared_ptr<ShardFactory> _shard_factory;

    std::shared_ptr<ShardManager> _shard_manager;

    std::shared_ptr<WorkerFactory> _worker_factory;

    std::shared_ptr<IWorkerManager> _worker_manager;
    std::atomic<bool> _worker_manager_ready = false;

    // spdlog::Logger& logger;
};

} // namespace donde_toolkits::feature_search::search_manager
