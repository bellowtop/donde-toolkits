#pragma once

#include "donde/feature_search/worker.h"

#include <iostream>
#include <unordered_map>

using namespace std;

namespace donde_toolkits ::feature_search ::search_manager {

// It's an interface, for the sake of compiling, we have default implements here.
// User should inherit this class.
class WorkerFactory {
  public:
    /*virtual*/ static Worker* CreateWorker(const std::string& worker_id,
                                            const std::string& worker_address) {
        throw "not implement";
    };
    /*virtual*/ static bool ProbeWorker(const std::string& worker_address) {
        throw "not implement";
    };
};

} // namespace donde_toolkits::feature_search::search_manager
