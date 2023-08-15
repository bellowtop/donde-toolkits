#include "worker_manager_impl.h"

#include "donde/definitions.h"
#include "donde/feature_search/definitions.h"
#include "donde/feature_search/driver.h"
#include "donde/feature_search/search_manager/worker_manager.h"
#include "donde/feature_search/worker.h"

#include <_types/_uint64_t.h>
#include <arm/types.h>
#include <cassert>
#include <chrono>
#include <iostream>
#include <memory>
#include <ostream>
#include <thread>
#include <unordered_map>

using namespace std;

namespace donde_toolkits ::feature_search ::search_manager {

WorkerManagerImpl::WorkerManagerImpl(Driver& driver) : _driver(driver) {
    auto workers_in_db = _driver.ListWorkers();
    for (auto& worker : workers_in_db) {
        _offline_workers.insert({worker.worker_id, worker});
    }
    _ping_thread = std::thread(&WorkerManagerImpl::ping_workers, std::ref(*this));
};

WorkerManagerImpl::~WorkerManagerImpl() {}

void WorkerManagerImpl::Stop() {
    stopped = true;
    for (const auto& it : _online_workers) {
        it.second->~Worker();
    }
    if (_ping_thread.joinable()) {
        _ping_thread.join();
    }
};

Worker* WorkerManagerImpl::FindWritableWorker() {
    uint64_t most_free = 0;
    std::string id;
    for (const auto& it : _online_workers) {
        auto space = it.second->GetFreeSpace();
        if (space > most_free) {
            most_free = space;
            id = it.first;
        }
    }
    if (id == "") {
        return nullptr;
    }
    return _online_workers[id];
};

void WorkerManagerImpl::AttachNewWorker(Worker* worker) {
    assert(worker != nullptr);
    std::string id = worker->GetWorkerID();

    auto found_offline = _offline_workers.find(id);
    if (found_offline != _offline_workers.end()) {
        std::cout << "an previously offline worker " << id << " now comes attched!" << std::endl;
        _offline_workers.erase(id);
    }

    auto found_online = _online_workers.find(id);
    if (found_online != _online_workers.end()) {
        _online_workers.insert({id, worker});
        std::cout << "worker " << id << " comes online: " << std::endl;

    } else {
        std::cerr << "already attached worker: " << id << std::endl;
    }
};
bool WorkerManagerImpl::AllWorkersOnline() { return _offline_workers.empty(); }

// TODO
Worker* WorkerManagerImpl::GetWorkerByID(const std::string& worker_id) { return {}; };

std::vector<Worker*> WorkerManagerImpl::ListWorkers(bool include_offline) { return {}; };

// Lazy boy's implementation: wait workers come up by themselves
void WorkerManagerImpl::LoadKnownWorkers(){};

void WorkerManagerImpl::ping_workers() {
    while (true) {
        if (stopped) {
            break;
        }
        if (!_offline_workers.empty()) {
            auto item = _offline_workers.begin();
            const std::string& id = item->first;
            const WorkerItem& worker = item->second;
            std::cout << "ping worker " << id << " with address: " << worker.address << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

} // namespace donde_toolkits::feature_search::search_manager
