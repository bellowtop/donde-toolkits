#pragma once

#include "donde/feature_search/driver.h"
#include "worker.h"

#include <iostream>
#include <memory>
#include <unordered_map>

using namespace std;

namespace donde_toolkits ::feature_search ::search_manager {

class IWorkerManager {
  public:
    virtual void Stop() = 0;

    virtual Worker* FindWritableWorker() = 0;

    virtual void AttachNewWorker(Worker* worker) = 0;

    virtual bool AllWorkersOnline() = 0;

    virtual void LoadKnownWorkers();
};

class WorkerManagerImpl;
class WorkerManager : public IWorkerManager {
  public:
    WorkerManager(Driver& driver);
    ~WorkerManager();

    void Stop() override;

    Worker* FindWritableWorker() override;
    void AttachNewWorker(Worker* worker) override;
    void LoadKnownWorkers() override;
    bool AllWorkersOnline() override;

    std::unique_ptr<WorkerManagerImpl> pimpl;
};

} // namespace donde_toolkits::feature_search::search_manager
