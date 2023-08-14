#include "remote_worker_impl.h"

namespace donde_toolkits ::feature_search ::search_manager {

RemoteWorkerImpl::RemoteWorkerImpl(){};
RemoteWorkerImpl::~RemoteWorkerImpl(){};

std::string RemoteWorkerImpl::GetWorkerID() { return {}; };

uint64 RemoteWorkerImpl::GetFreeSpace() { return {}; };

// ListShards report all shards this worker is serving.
std::vector<DBShard> RemoteWorkerImpl::ListShards() { return {}; };

// ServeShard let the worker serve this shard, for its features' CRUD
RetCode RemoteWorkerImpl::ServeShard(Shard& shard) { return {}; };

// CloseShard close db_id/shard_id.
RetCode RemoteWorkerImpl::CloseShard(const std::string& db_id, const std::string& shard_id) {
    return {};
};

// AddFeatures to db_id/shard_id, delegate to remote worker.
std::vector<std::string> AddFeatures(const std::string& db_id, const std::string& shard_id,
                                     const std::vector<Feature>& fts) {
    return {};
};

// Search feature in the worker. worker can have multiple dbs, multiple shards.
// only search in the requested db.
std::vector<FeatureSearchItem> SearchFeature(const std::string& db_id, const Feature& query,
                                             int topk) {
    return {};
};

} // namespace donde_toolkits::feature_search::search_manager
