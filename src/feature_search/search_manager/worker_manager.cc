#include "donde/feature_search/search_manager/worker_manager.h"

#include "donde/feature_search/worker.h"
#include "worker_manager_impl.h"

#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

using namespace std;

namespace donde_toolkits ::feature_search ::search_manager {

WorkerManager::WorkerManager(Driver& driver) : pimpl(std::make_unique<WorkerManagerImpl>(driver)){};
WorkerManager::~WorkerManager(){};
Worker* WorkerManager::FindWritableWorker() { return pimpl->FindWritableWorker(); };
void WorkerManager::Stop() { return pimpl->Stop(); };
void WorkerManager::AttachNewWorker(WorkerItem worker_item) {
    return pimpl->AttachNewWorker(worker_item);
};
void WorkerManager::LoadKnownWorkers() { pimpl->LoadKnownWorkers(); };
std::vector<Worker*> WorkerManager::ListWorkers(bool include_offline) {
    return pimpl->ListWorkers(include_offline);
};

} // namespace donde_toolkits::feature_search::search_manager
