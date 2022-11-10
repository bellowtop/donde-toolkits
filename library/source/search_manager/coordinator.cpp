#include "search_manager/coordinator.h"

#include "fmt/format.h"
#include "search/definitions.h"
#include "search/impl/simple_driver.h"
#include "search_manager/shard_manager.h"
#include "search_manager/worker_client.h"
#include "types.h"

#include <exception>
#include <memory>
#include <spdlog/spdlog.h>

Coordinator::Coordinator(const json& coor_config) : config(coor_config) {
    if (!coor_config.contains("workers")) {
        throw "json config missing workers list.";
    }
    if (!coor_config.contains("driver")) {
        throw "json config missing driver config.";
    }
    std::string driver_type = coor_config["driver"];
    if (driver_type != search::SEARCH_DRIVER_CASSANDRA
        || driver_type != search::SEARCH_DRIVER_SIMPLE) {
        throw fmt::format("driver {} is not supported.", driver_type);
    }

    // if (driver_type == search::SEARCH_DRIVER_CASSANDRA) {
    _driver = std::make_shared<search::SimpleDriver>((std::string)coor_config["cassandra"]["addr"]);
    // }

    _shard_manager = std::make_shared<ShardManager>(*(_driver.get()));

    _worker_addrs = (std::vector<std::string>)coor_config["workers"];
};

Coordinator::~Coordinator(){};

void Coordinator::Start() {

    initialize_workers();

    assign_worker_for_shards();
};

void Coordinator::Stop(){};

// AddFeatures to this db, we need find proper shard to store these fts.
RetCode Coordinator::AddFeatures(const std::string& db_id, const std::vector<Feature>& fts) {
    auto [shard, new_created] = _shard_manager->FindOrCreateWritableShard(db_id, fts.size());

    // if newly created shard, it doesn't have a worker.
    if (new_created) {
        Worker* worker = find_worker_for_shard(shard);
        if (worker == nullptr) {
            spdlog::error("cannot find a worker for shard: {}", shard->GetShardID());
            return RetCode::RET_ERR;
        }
        shard->AssignWorker(worker);
        worker->ServeShard(shard->GetShardInfo());
    }

    shard->AddFeatures(fts);

    return {};
};

// SearchFeatures in this db.
std::vector<Feature> Coordinator::SearchFeature(const std::string& db_id, const Feature& query,
                                                int topk) {
    return {};
};

std::vector<Feature> Coordinator::SearchFeatureInTimePeriod(const std::string& db_id,
                                                            const Feature& query, int topk) {
    return {};
};

Worker* Coordinator::find_worker_for_shard(Shard* shard) {
    int64 free_space = INT_MIN;
    Worker* selected;

    for (auto& worker : _workers) {
        if (worker->GetFreeSpace() > free_space) {
            selected = worker.get();
            free_space = worker->GetFreeSpace();
        }
    }

    return selected;
};

void Coordinator::assign_worker_for_shards() {
    std::vector<search::DBItem> dbs = _shard_manager->ListUserDBs();
    for (auto& db : dbs) {
        auto shards = _shard_manager->ListShards(db.db_id);
        for (auto& shard : shards) {
            Worker* worker = find_worker_for_shard(shard);
            if (worker == nullptr) {
                spdlog::error("cannot find a worker for shard: {}", shard->GetShardID());
                continue;
            }
            shard->AssignWorker(worker);
            worker->ServeShard(shard->GetShardInfo());
        }
    }
};

void Coordinator::initialize_workers() {
    for (auto& addr : _worker_addrs) {
        try {
            // may has exception, we handled bellow.
            auto client = std::make_shared<WorkerClient>(addr);
            _workers.push_back(client);
        } catch (const std::exception& exc) {
            _invalid_worker_addrs.push_back(addr);
        }
    }

    if (_invalid_worker_addrs.size() > 0) {
        spdlog::warn("supplied worker addrs has some wrong addrs, we cannot connect to them:");
        for (auto& v : _invalid_worker_addrs) {
            spdlog::warn("{}", v);
        }
    }
};
