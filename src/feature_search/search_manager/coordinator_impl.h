#pragma once

#include "donde/feature_search/driver.h"
#include "donde/feature_search/search_manager/coordinator.h"
#include "donde/feature_search/search_manager/worker.h"
#include "donde/feature_search/search_manager/worker_manager.h"
#include "shard.h"
#include "shard_factory.h"

// #include "spdlog/spdlog.h"

#include <algorithm>
#include <cstdio>
#include <iostream>
#include <thread>
#include <unordered_map>

using namespace std;

namespace donde_toolkits ::feature_search ::search_manager {

using WorkerPtr = std::shared_ptr<Worker>;

// Coordinator & Reducer
class CoordinatorImpl {
  public:
    CoordinatorImpl(const json& coor_config);
    ~CoordinatorImpl();

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

  private:
    void initialize_workers();
    void deinitialize_workers();
    void assign_worker_for_shards();

    void load_user_dbs();

  private:
    const json& config;

    std::vector<DBItem> _db_items;

    std::vector<std::string> _worker_addrs;
    std::vector<std::string> _invalid_worker_addrs;

    std::unordered_map<std::string, std::string> _managed_workers;

    shared_ptr<Driver> _driver;

    std::shared_ptr<ShardFactory> _shard_factory;

    std::shared_ptr<ShardManager> _shard_manager;

    std::shared_ptr<IWorkerManager> _worker_manager;
    std::thread _worker_ready_thread;
    bool _worker_manager_ready = false;
    void check_worker_manager_ready() {
        while (true) {
            bool ok = _worker_manager->AllWorkersOnline();
            if (ok) {
                _worker_manager_ready = true;
                return;
            }
            std::cout << "worker manager still no ready..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    // spdlog::Logger& logger;
};

} // namespace donde_toolkits::feature_search::search_manager
