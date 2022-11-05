#pragma once

#include "config.h"
#include "search/db_searcher.h"
#include "search/definitions.h"
#include "shard_manager.h"
#include "worker_client.h"

// #include "spdlog/spdlog.h"

#include <iostream>
#include <unordered_map>

using namespace std;

class Coordinator {
  public:
    Coordinator(Config& coor_config);
    ~Coordinator();

    void Start();

    void Stop();

    RetCode ConnectWorkers();

    std::vector<Worker*> ListWorkers();

    std::vector<search::DBItem*> ListUserDBs();

    RetCode LoadUserDBs();

    // AddFeatures to this db, we need find proper shard to store these fts.
    RetCode AddFeatures(const std::string& db_id, const std::vector<Feature>& fts);

    // SearchFeatures in this db.
    std::vector<Feature> SearchFeature(const std::string& db_id, const Feature& query, int topk);

  private:
    Config& config;
    // spdlog::Logger& logger;
};
