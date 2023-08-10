#pragma once

#include "donde/definitions.h"
#include "donde/feature_search/definitions.h"
#include "donde/feature_search/search_manager/worker.h"
#include "donde/feature_search/search_manager/worker_manager.h"

#include <iostream>
#include <memory>
#include <unordered_map>

using namespace std;

namespace donde_toolkits ::feature_search ::search_manager {

class WorkerManagerImpl : public IWorkerManager {
  public:
    WorkerManagerImpl();
    ~WorkerManagerImpl();

    std::shared_ptr<Worker> FindWritableWorker() override;
    void AttachWorker(std::shared_ptr<Worker> worker) override;

  private:
    std::unordered_map<std::string, std::shared_ptr<Worker>> online_workers;
};

} // namespace donde_toolkits::feature_search::search_manager
