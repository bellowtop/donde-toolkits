#pragma once

#include "config.h"
#include "search/db_searcher.h"
#include "search/definitions.h"
#include "search/impl/cassandra_driver.h"
#include "search_manager/shard_manager.h"
#include "search_manager/worker_client.h"

// #include "spdlog/spdlog.h"

#include <iostream>
#include <unordered_map>

using namespace std;

using WorkerPtr = shared_ptr<Worker>;

// Coordinator & Reducer
class Coordinator {
  public:
    Coordinator(const json& coor_config);
    ~Coordinator();

    void Start();

    void Stop();

    // std::vector<WorkerPtr> ListWorkers();

    std::vector<search::DBItem> ListUserDBs();

    // AddFeatures to this db, we need find proper shard to store these fts.
    RetCode AddFeatures(const std::string& db_id, const std::vector<Feature>& fts);

    // SearchFeatures in this db.
    std::vector<Feature> SearchFeature(const std::string& db_id, const Feature& query, int topk);

    std::vector<Feature> SearchFeatureInTimePeriod(const std::string& db_id, const Feature& query,
                                                   int topk);

  private:
    RetCode initialize_workers();
    Worker* find_worker_for_shard(Shard* shard);
    RetCode assign_worker_for_shards();

  private:
    const json& config;

    std::vector<std::string> _worker_addrs;
    std::vector<std::string> _invalid_worker_addrs;

    std::vector<WorkerPtr> _workers;

    shared_ptr<search::Driver> _driver;

    std::shared_ptr<ShardManager> _shard_manager;

    // spdlog::Logger& logger;
};
