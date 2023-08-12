#include "donde/feature_search/search_manager/worker_manager.h"

#include "donde/feature_search/search_manager/worker.h"
#include "worker_manager_impl.h"

#include <iostream>
#include <memory>
#include <unordered_map>

using namespace std;

namespace donde_toolkits ::feature_search ::search_manager {

WorkerManager::WorkerManager(Driver& driver) : pimpl(std::make_unique<WorkerManagerImpl>(driver)){};
WorkerManager::~WorkerManager(){};
Worker* WorkerManager::FindWritableWorker() { return pimpl->FindWritableWorker(); };
void WorkerManager::Stop() { return pimpl->Stop(); };
void WorkerManager::AttachNewWorker(Worker* worker) { return pimpl->AttachNewWorker(worker); };
void WorkerManager::LoadKnownWorkers() { pimpl->LoadKnownWorkers(); };
bool WorkerManager::AllWorkersOnline() { return pimpl->AllWorkersOnline(); };

} // namespace donde_toolkits::feature_search::search_manager
