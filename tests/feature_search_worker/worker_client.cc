
#include "donde/definitions.h"
#include "donde/utils.h"
#include "source/feature_search/search_manager/worker_impl.h"

#include <chrono>
#include <filesystem>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <ostream>
#include <thread>

using namespace std;

using nlohmann::json;

TEST(SearchWorkerTest, Basic) {

    // Expect two strings not to be equal.
    EXPECT_STRNE("hello", "world");
    // Expect equality.
    EXPECT_EQ(7 * 6, 42);

    // WorkerClient client{""};

    // // std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    // client.DisConnect();

    // CHECK("aa" == "aa");
};
