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

WorkerManager::WorkerManager() : pimpl(std::make_unique<WorkerManagerImpl>()){};
WorkerManager::~WorkerManager(){};
std::shared_ptr<Worker> WorkerManager::FindWritableWorker() { return pimpl->FindWritableWorker(); };
void WorkerManager::AttachWorker(std::shared_ptr<Worker> worker) {
    return pimpl->AttachWorker(worker);
};

} // namespace donde_toolkits::feature_search::search_manager
