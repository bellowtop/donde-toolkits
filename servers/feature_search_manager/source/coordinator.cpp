#include "coordinator.h"

#include "fmt/format.h"
#include "gen/pb-cpp/feature_search_inner.grpc.pb.h"
#include "search/definitions.h"
#include "shard_manager.h"
#include "types.h"
#include "worker_client.h"

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
    _driver
        = std::make_shared<search::CassandraDriver>((std::string)coor_config["cassandra"]["addr"]);
    // }

    _shard_manager = std::make_shared<ShardManager>(*(_driver.get()));

    _worker_addrs = (std::vector<std::string>)coor_config["workers"];
};

Coordinator::~Coordinator(){};

void Coordinator::Start() { initialize_workers(); };

void Coordinator::Stop(){};

RetCode Coordinator::initialize_workers() {
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

    return {};
};

// AddFeatures to this db, we need find proper shard to store these fts.
RetCode Coordinator::AddFeatures(const std::string& db_id, const std::vector<Feature>& fts) {
    Shard* shard = _shard_manager->FindOrCreateWritableShard(db_id);
    if (!shard->HasWorker()) {
        Worker* worker = find_worker_for_shard();
        if (worker == nullptr) {
            spdlog::error("cannot find a worker for shard: {}", shard->GetShardID());
            return RetCode::RET_ERR;
        }
        shard->AssignWorker(worker);
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

Worker* Coordinator::find_worker_for_shard() { return {}; };
