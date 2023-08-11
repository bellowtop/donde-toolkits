#include "donde/feature_search/search_manager/worker_manager.h"

#include "donde/definitions.h"
#include "donde/feature_search/definitions.h"
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
void WorkerManager::AttachWorker(Worker* worker) { return pimpl->AttachWorker(worker); };

} // namespace donde_toolkits::feature_search::search_manager
