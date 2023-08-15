#include "donde/feature_search/search_manager/remote_worker.h"

#include "donde/feature_search/shard.h"
#include "remote_worker_impl.h"

namespace donde_toolkits ::feature_search ::search_manager {

RemoteWorker::RemoteWorker(){};
RemoteWorker::~RemoteWorker(){};

std::string RemoteWorker::GetWorkerID() { return pimpl->GetWorkerID(); };

std::string RemoteWorker::GetAddress() { return pimpl->GetAddress(); };

bool RemoteWorker::Ready() { return pimpl->Ready(); };

uint64 RemoteWorker::GetFreeSpace() { return pimpl->GetFreeSpace(); };

// ListShards report all shards this worker is serving.
std::vector<DBShard> RemoteWorker::ListShards() { return pimpl->ListShards(); };

// ServeShard let the worker serve this shard, for its features' CRUD
RetCode RemoteWorker::ServeShard(Shard& shard) { return pimpl->ServeShard(shard); };

// CloseShard close db_id/shard_id.
RetCode RemoteWorker::CloseShard(const std::string& db_id, const std::string& shard_id) {
    return pimpl->CloseShard(db_id, shard_id);
};

// AddFeatures to db_id/shard_id, delegate to remote worker.
std::vector<std::string> RemoteWorker::AddFeatures(const std::string& db_id,
                                                   const std::string& shard_id,
                                                   const std::vector<Feature>& fts) {
    return pimpl->AddFeatures(db_id, shard_id, fts);
};

// Search feature in the worker. worker can have multiple dbs, multiple shards.
// only search in the requested db.
std::vector<FeatureSearchItem> RemoteWorker::SearchFeature(const std::string& db_id,
                                                           const Feature& query, int topk) {
    return pimpl->SearchFeature(db_id, query, topk);
};

} // namespace donde_toolkits::feature_search::search_manager
