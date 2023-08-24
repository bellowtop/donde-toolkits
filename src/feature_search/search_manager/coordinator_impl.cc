#include "coordinator_impl.h"

#include "donde/definitions.h"
#include "donde/feature_search/definitions.h"
#include "donde/feature_search/feature_topk_rank.h"
#include "donde/feature_search/search_manager/coordinator.h"
#include "donde/feature_search/search_manager/worker_factory.h"
#include "donde/feature_search/shard.h"
#include "donde/feature_search/simple_driver.h"
#include "donde/feature_search/worker.h"
#include "fmt/format.h"
#include "shard_impl.h"
#include "shard_manager_impl.h"
#include "worker_manager_impl.h"

#include <exception>
#include <memory>
#include <queue>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>

using namespace fmt;

namespace donde_toolkits ::feature_search ::search_manager {

CoordinatorImpl::CoordinatorImpl(const json& coor_config) : config(coor_config) {
    if (!coor_config.contains("workers")) {
        throw "json config missing workers list.";
    }
    if (!coor_config.contains("driver")) {
        throw "json config missing driver config.";
    }
    std::string driver_type = coor_config["driver"];
    if (driver_type != SEARCH_DRIVER_CASSANDRA || driver_type != SEARCH_DRIVER_SIMPLE) {
        throw fmt::format("driver {} is not supported.", driver_type);
    }

    _driver = std::make_shared<SimpleDriver>((std::string)coor_config["cassandra"]["addr"]);
    _shard_factory = std::make_shared<ShardFactoryImpl>();
    _shard_manager = std::make_shared<ShardManagerImpl>(*_driver, *_shard_factory);

    // TODO, how to set worker factory & manager?
    // _worker_factory = std::make_shared<WorkerFactory>();
    // _worker_manager = std::make_shared<WorkerManagerImpl>(*_driver, *_worker_factory);

    // check worker_maanger is ready in the background.
    std::thread([&]() mutable {
        while (true) {
            auto workers = _worker_manager->ListWorkers(true);
            bool all_ready = true;
            for (const auto& w : workers) {
                if (!w->Ready()) {
                    all_ready = false;
                    break;
                }
            }
            if (all_ready) {
                std::cout << "worker manager is ready!" << std::endl;
                _worker_manager_ready.store(true);
                return;
            }
            std::cout << "worker manager still no ready..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }).detach();
};

CoordinatorImpl::~CoordinatorImpl(){};

void CoordinatorImpl::Start() {
    load_user_dbs();
    load_known_shards();
};

void CoordinatorImpl::Stop() { deinitialize_workers(); };

std::vector<DBItem> CoordinatorImpl::ListUserDBs() { return {}; };

// AddFeatures to this db, we need find proper shard to store these fts.
std::vector<std::string> CoordinatorImpl::AddFeatures(const std::string& db_id,
                                                      const std::vector<Feature>& fts) {
    auto [shard, new_created] = _shard_manager->FindOrCreateWritableShard(db_id, fts.size());

    // if newly created shard, it doesn't have a worker.
    if (new_created) {
        Worker* worker = _worker_manager->FindWritableWorker();
        if (worker == nullptr) {
            spdlog::error("cannot find a worker for shard: {}", shard->GetShardID());
            return {};
        }
        worker->ServeShard(*shard);
    }

    return shard->AddFeatures(fts);
};

// RemoveFeatures from this db
RetCode CoordinatorImpl::RemoveFeatures(const std::string& db_id,
                                        const std::vector<std::string>& feature_ids) {
    // TODO
    return RetCode::RET_OK;
};

// SearchFeatures in this db. delegate to shards.
std::vector<FeatureSearchItem> CoordinatorImpl::SearchFeature(const std::string& db_id,
                                                              const Feature& query, int topk) {
    // enlarge search area
    int enlarge_topk = topk * 2;

    FeatureTopkRanking rank(query, topk);

    auto shards = _shard_manager->ListShards(db_id);

    // TODO group shards to search at once.
    // a db has many shards, we should not search one by one, as each search establish a worker
    // connection.
    for (auto& shard : shards) {
        auto searched = shard->SearchFeature(query, enlarge_topk);
        rank.FeedIn(searched);
    }

    std::vector<FeatureSearchItem> ret = rank.SortOut();

    return ret;
};

void CoordinatorImpl::load_user_dbs() { _db_items = _driver->ListDBs(); }

void CoordinatorImpl::load_known_shards() {
    std::vector<Shard*> known_shards;
    for (auto& db : _db_items) {
        auto shards = _shard_manager->ListShards(db.db_id);
        known_shards.insert(known_shards.end(), shards.begin(), shards.end());
    }

    // start a new thread to check.
    // note we capture the vector by reference.
    std::thread async_check_known_shards([&]() mutable {
        for (auto it = known_shards.begin(); it != known_shards.end(); it++) {
            auto shard = *it;
            Worker* worker = _worker_manager->GetWorkerByID(shard->GetShardInfo().worker_id);
            if (worker == nullptr) {
                spdlog::error("cannot find a worker for shard: {}", shard->GetShardID());
                continue;
            }
            if (!worker->Ready()) {
                spdlog::error("worker: {} addr: {} is not ready.", worker->GetWorkerID(),
                              worker->GetAddress());
                continue;
            }
            // bidirectional bound.
            worker->ServeShard(*shard);
            shard->AssignWorker(worker);
            known_shards.erase(it);
        }
    });
    // as the operation is not heavy loaded, just detach it, let it running foever
    // (it will stop by itself.)
    async_check_known_shards.detach();
};

} // namespace donde_toolkits::feature_search::search_manager
