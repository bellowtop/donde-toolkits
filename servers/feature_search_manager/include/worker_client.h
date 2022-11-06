#pragma once

#include "types.h"
#include "utils.h"
#include "worker_api.h"

#include <string>
#include <vector>

class WorkerClient : public Worker {
  public:
    WorkerClient(const std::string& addr) : _worker_id(generate_uuid()), _addr(addr){};

    // Connect to remote addr, and regularly check liveness.
    RetCode Connect();

    inline std::string GetWorkerID() { return _worker_id; };

    // WorkerAPI implement
    RetCode AddFeatures(const std::string& db_id, const std::string& shard_id,
                        const std::vector<Feature>& fts);

    std::vector<Feature> SearchFeature(const std::string& db_id, const Feature& query, int topk);

  public:
    std::string _worker_id;
    std::string _addr;

    bool _connected;

    bool _is_writing;

    std::vector<std::string> _shard_ids;

    size_t _capacity;
    size_t _used;
};
