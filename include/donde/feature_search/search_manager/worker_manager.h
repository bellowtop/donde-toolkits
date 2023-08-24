#pragma once

#include "donde/feature_search/driver.h"
#include "donde/feature_search/search_manager/worker_factory.h"
#include "donde/feature_search/worker.h"

#include <iostream>
#include <memory>
#include <unordered_map>

using namespace std;

namespace donde_toolkits ::feature_search ::search_manager {

class IWorkerManager {
  public:
    virtual void Stop() = 0;

    virtual Worker* FindWritableWorker() = 0;

    virtual Worker* GetWorkerByID(const std::string& worker_id) = 0;

    virtual void AttachNewWorker(WorkerItem worker_item) = 0;

    virtual std::vector<Worker*> ListWorkers(bool include_offline = false) = 0;

    virtual void LoadKnownWorkers();
};

class WorkerManagerImpl;
class WorkerManager : public IWorkerManager {
  public:
    WorkerManager(Driver& driver, WorkerFactory& factory);
    ~WorkerManager();

    void Stop() override;

    Worker* FindWritableWorker() override;
    void AttachNewWorker(WorkerItem worker_item) override;
    void LoadKnownWorkers() override;
    std::vector<Worker*> ListWorkers(bool include_offline = false) override;

    std::unique_ptr<WorkerManagerImpl> pimpl;
};

} // namespace donde_toolkits::feature_search::search_manager
