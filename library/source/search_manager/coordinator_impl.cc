#include "coordinator_impl.h"

#include "../search/simple_driver.h"
#include "definitions.h"
#include "fmt/format.h"
#include "search/definitions.h"
#include "search_manager/api.h"
#include "shard_manager_impl.h"

#include <exception>
#include <memory>
#include <queue>
#include <spdlog/spdlog.h>

CoordinatorImpl::CoordinatorImpl(const json& coor_config) : config(coor_config) {
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

    _shard_manager = std::make_shared<ShardManagerImpl>(*(_driver.get()));

    _worker_addrs = (std::vector<std::string>)coor_config["workers"];
};

CoordinatorImpl::~CoordinatorImpl(){};

void CoordinatorImpl::Start() {

    initialize_workers();

    assign_worker_for_shards();
};

void CoordinatorImpl::Stop() { deinitialize_workers(); };

// AddFeatures to this db, we need find proper shard to store these fts.
RetCode CoordinatorImpl::AddFeatures(const std::string& db_id, const std::vector<Feature>& fts) {
    auto [shard, new_created] = _shard_manager->FindOrCreateWritableShard(db_id, fts.size());

    // if newly created shard, it doesn't have a worker.
    if (new_created) {
        Worker* worker = find_worker_for_shard(shard);
        if (worker == nullptr) {
            spdlog::error("cannot find a worker for shard: {}", shard->GetShardID());
            return RetCode::RET_ERR;
        }
        _shard_manager->AssignWorkerToShard(shard, worker);
        // shard->AssignWorker(worker);
        // worker->ServeShard(shard->GetShardInfo());
    }

    shard->AddFeatures(fts);

    return RetCode::RET_OK;
};

// SearchFeatures in this db.
std::vector<FeatureScore> CoordinatorImpl::SearchFeature(const std::string& db_id,
                                                         const Feature& query, int topk) {
    // use min heap to sort topk
    std::priority_queue<FeatureScore, std::vector<FeatureScore>, FeatureScoreComparator> min_heap;

    // enlarge search area
    int enlarge_topk = topk * 2;

    auto shards = _shard_manager->ListShards(db_id);

    // TODO group shards to search at once.
    // a db has many shards, we should not search one by one, as each search trigger worker
    // connection.
    for (auto& shard : shards) {
        auto searched = shard->SearchFeature(query, enlarge_topk);
        for (auto& ft_score : searched) {
            if (min_heap.size() < topk) {
                min_heap.push(ft_score);
                continue;
            }
            if (min_heap.top().score < ft_score.score) {
                min_heap.pop();
                min_heap.push(ft_score);
            }
        }
    }

    std::vector<FeatureScore> ret;
    while (!min_heap.empty()) {
        ret.push_back(min_heap.top());
        min_heap.pop();
    }

    return ret;
};

Worker* CoordinatorImpl::find_worker_for_shard(Shard* shard) {
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

void CoordinatorImpl::assign_worker_for_shards() {
    std::vector<search::DBItem> dbs = _shard_manager->ListUserDBs();

    for (auto& db : dbs) {
        auto shards = _shard_manager->ListShards(db.db_id);
        for (auto& shard : shards) {
            Worker* worker = find_worker_for_shard(shard);
            if (worker == nullptr) {
                spdlog::error("cannot find a worker for shard: {}", shard->GetShardID());
                continue;
            }
            _shard_manager->AssignWorkerToShard(shard, worker);
            // shard->AssignWorker(worker);
            // worker->ServeShard(shard->GetShardInfo());
        }
    }
};

void CoordinatorImpl::initialize_workers() {
    for (auto& addr : _worker_addrs) {
        try {
            // may has exception, we handled bellow.
            auto client = std::make_shared<WorkerImpl>(addr);
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

void CoordinatorImpl::deinitialize_workers(){};
