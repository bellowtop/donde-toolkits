#include "donde/feature_search/search_worker/brute_force_worker.h"

#include "brute_force_worker_impl.h"
#include "donde/definitions.h"
#include "donde/feature_search/feature_topk_rank.h"
#include "mem_shard_impl.h"
#include "shard.h"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <queue>
#include <spdlog/spdlog.h>



using json = nlohmann::json;

namespace donde_toolkits ::feature_search ::search_worker {

BruteForceWorker::BruteForceWorker(Driver& driver)
    : ISearchWorker(), pimpl(std::make_unique<BruteForceWorkerImpl>(driver)){};

BruteForceWorker::~BruteForceWorker(){};

RetCode BruteForceWorker::ServeShards(const std::vector<DBShard>& shard_infos) {
    return pimpl->ServeShards(shard_infos);
};

RetCode BruteForceWorker::CloseShards(const std::vector<DBShard>& shard_infos) {
    return pimpl->CloseShards(shard_infos);
};

std::vector<FeatureSearchItem> BruteForceWorker::SearchFeature(const std::string& db_id,
                                                               const Feature& query, size_t topk) {
    return pimpl->SearchFeature(db_id, query, topk);
};

std::vector<std::string> BruteForceWorker::AddFeatures(const std::string& db_id,
                                                       const std::vector<FeatureDbItem>& features) {
    return pimpl->AddFeatures(db_id, features);
};

RetCode BruteForceWorker::RemoveFeatures(const std::string& db_id,
                                         const std::vector<std::string>& feature_ids) {
    return pimpl->RemoveFeatures(db_id, feature_ids);
};

} // namespace donde_toolkits::feature_search::search_worker
