#include "worker_client.h"

// Connect to remote addr, and regularly check liveness.
RetCode WorkerClient::Connect() { return {}; };

// WorkerAPI implement
RetCode WorkerClient::AddFeatures(const std::string& db_id, const std::string& shard_id,
                                  const std::vector<Feature>& fts) {
    return {};
};

std::vector<Feature> WorkerClient::SearchFeature(const std::string& db_id, const Feature& query,
                                                 int topk) {
    return {};
};
