#include "donde/feature_search/search_manager/coordinator.h"

#include "coordinator_impl.h"
#include "donde/definitions.h"
#include "donde/feature_search/feature_topk_rank.h"
#include "donde/feature_search/simple_driver.h"

#include <exception>
#include <memory>
#include <queue>
#include <spdlog/spdlog.h>

using namespace fmt;

namespace donde_toolkits ::feature_search ::search_manager {

Coordinator::Coordinator(const json& coor_config)
    : pimpl(std::make_unique<CoordinatorImpl>(coor_config)){};

Coordinator::~Coordinator(){};

void Coordinator::Start() { return pimpl->Start(); };

void Coordinator::Stop() { return pimpl->Stop(); };

void Coordinator::ProbeWorkers(const WorkerFactory& factory) {
    return pimpl->ProbeWorkers(factory);
};

std::vector<DBItem> Coordinator::ListUserDBs() { return pimpl->ListUserDBs(); };

// AddFeatures to this db, we need find proper shard to store these fts.
std::vector<std::string> Coordinator::AddFeatures(const std::string& db_id,
                                                  const std::vector<Feature>& fts) {
    return pimpl->AddFeatures(db_id, fts);
};

// RemoveFeatures from this db
RetCode Coordinator::RemoveFeatures(const std::string& db_id,
                                    const std::vector<std::string>& feature_ids) {
    return pimpl->RemoveFeatures(db_id, feature_ids);
};

// SearchFeatures in this db. delegate to shards.
std::vector<FeatureSearchItem> Coordinator::SearchFeature(const std::string& db_id,
                                                          const Feature& query, int topk) {
    return pimpl->SearchFeature(db_id, query, topk);
};

} // namespace donde_toolkits::feature_search::search_manager
