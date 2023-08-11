#pragma once

#include "donde/definitions.h"
#include "donde/feature_search/definitions.h"
#include "donde/feature_search/driver.h"
#include "worker.h"

#include <iostream>
#include <memory>
#include <unordered_map>

using namespace std;

namespace donde_toolkits ::feature_search ::search_manager {

class IWorkerManager {
  public:
    virtual Worker* FindWritableWorker() = 0;

    virtual void AttachWorker(Worker* worker) = 0;

    virtual void LoadKnownWorkers(std::unordered_map<std::string, std::string> known_workers);
};

class WorkerManagerImpl;
class WorkerManager : public IWorkerManager {
  public:
    WorkerManager(Driver& driver);
    ~WorkerManager();

    Worker* FindWritableWorker() override;
    void AttachWorker(Worker* worker) override;

    std::unique_ptr<WorkerManagerImpl> pimpl;
};

} // namespace donde_toolkits::feature_search::search_manager
