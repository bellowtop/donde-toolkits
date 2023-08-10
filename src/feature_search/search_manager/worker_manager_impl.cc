#include "worker_manager_impl.h"

#include "donde/definitions.h"
#include "donde/feature_search/definitions.h"
#include "donde/feature_search/search_manager/worker.h"

#include <_types/_uint64_t.h>
#include <iostream>
#include <memory>
#include <unordered_map>

using namespace std;

namespace donde_toolkits ::feature_search ::search_manager {

WorkerManagerImpl::WorkerManagerImpl(){};
WorkerManagerImpl::~WorkerManagerImpl(){};

std::shared_ptr<Worker> WorkerManagerImpl::FindWritableWorker() {
    uint64_t most_free = 0;
    std::string id;
    for (auto& it : online_workers) {
        auto space = it.second->GetFreeSpace();
        if (space > most_free) {
            most_free = space;
            id = it.first;
        }
    }
    if (id == "") {
        return nullptr;
    }
    return online_workers[id];
};

void WorkerManagerImpl::AttachWorker(std::shared_ptr<Worker> worker) {
    std::string id = worker->GetWorkerID();
    auto found = online_workers.find(id);
    if (found != online_workers.end()) {
        online_workers.insert({id, worker});
    } else {
        std::cerr << "already attached worker: " << id << std::endl;
    }
};

} // namespace donde_toolkits::feature_search::search_manager
