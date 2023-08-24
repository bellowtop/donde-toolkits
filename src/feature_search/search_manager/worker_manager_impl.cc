#include "worker_manager_impl.h"

#include "donde/definitions.h"
#include "donde/feature_search/definitions.h"
#include "donde/feature_search/driver.h"
#include "donde/feature_search/search_manager/worker_manager.h"
#include "donde/feature_search/worker.h"
#include "remote_worker_impl.h"

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

WorkerManagerImpl::WorkerManagerImpl(Driver& driver, WorkerFactory& factory)
    : _driver(driver), _factory(factory) {
    auto workers_in_db = _driver.ListWorkers();
    for (auto& worker : workers_in_db) {
        _known_workers.insert({worker.worker_id, worker});
    }
    _ping_thread = std::thread(&WorkerManagerImpl::ping_workers, std::ref(*this));
};

WorkerManagerImpl::~WorkerManagerImpl() {}

void WorkerManagerImpl::Stop() {
    stopped = true;
    for (const auto& it : _online_workers) {
        // free Worker*, they are created by factory
        delete it.second;
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

void WorkerManagerImpl::AttachNewWorker(WorkerItem worker_item) {
    std::string id = worker_item.worker_id;

    auto found_offline = _known_workers.find(id);
    if (found_offline != _known_workers.end()) {
        std::cerr << "this worker " << id << " is already known." << std::endl;
        return;
    }

    _known_workers.insert({id, worker_item});
};

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
        if (!_known_workers.empty()) {
            auto item = _known_workers.begin();
            const std::string& id = item->first;
            const WorkerItem& worker = item->second;
            std::cout << "ping worker " << id << " with address: " << worker.address << std::endl;
            // if ping success
            if (_factory.ProbeWorker(worker.address)) {
                _known_workers.erase(id);
                _online_workers.insert(
                    {id, _factory.CreateWorker(worker.worker_id, worker.address)});
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

} // namespace donde_toolkits::feature_search::search_manager
