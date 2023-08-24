#pragma once

#include "donde/feature_search/worker.h"

#include <iostream>
#include <unordered_map>

using namespace std;

namespace donde_toolkits ::feature_search ::search_manager {

class WorkerFactory {
  public:
    virtual ~WorkerFactory() = default;
    virtual Worker* CreateWorker(const std::string& worker_id, const std::string& worker_address)
        = 0;
    virtual bool ProbeWorker(const std::string& worker_address) = 0;
};

} // namespace donde_toolkits::feature_search::search_manager
