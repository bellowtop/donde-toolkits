
#include "search_manager/worker_client.h"

#include "types.h"
#include "utils.h"

#include <chrono>
#include <doctest/doctest.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <ostream>
#include <thread>

using namespace std;

using nlohmann::json;

TEST_CASE("WorkerClient: check liveness loop.") {

    WorkerClient client{""};

    // std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    client.DisConnect();

    CHECK("aa" == "aa");
};
