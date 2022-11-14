#pragma once

#include "../search/cassandra_driver.h"
#include "config.h"
#include "search/api.h"
#include "search_manager/api.h"

// #include "spdlog/spdlog.h"

#include <iostream>
#include <unordered_map>

using namespace std;

using WorkerPtr = shared_ptr<Worker>;

// Coordinator & Reducer
class CoordinatorImpl : public Coordinator {
  public:
    CoordinatorImpl(const json& coor_config);
    ~CoordinatorImpl();

    void Start();

    void Stop();

    // std::vector<WorkerPtr> ListWorkers();

    std::vector<search::DBItem> ListUserDBs();

    // AddFeatures to this db, we need find proper shard to store these fts.
    RetCode AddFeatures(const std::string& db_id, const std::vector<Feature>& fts);

    // SearchFeatures in this db.
    std::vector<FeatureScore> SearchFeature(const std::string& db_id, const Feature& query,
                                            int topk);

  private:
    void initialize_workers();
    void deinitialize_workers();
    void assign_worker_for_shards();
    Worker* find_worker_for_shard(Shard* shard);

  private:
    const json& config;

    std::vector<std::string> _worker_addrs;
    std::vector<std::string> _invalid_worker_addrs;

    std::vector<WorkerPtr> _workers;

    shared_ptr<search::Driver> _driver;

    std::shared_ptr<ShardManager> _shard_manager;

    // spdlog::Logger& logger;
};
