#include "definitions.h"
#include "search/impl/brute_force_searcher.h"
#include "search/impl/simple_driver.h"
#include "utils.h"

#include <cstdlib>
#include <filesystem>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <ostream>

using namespace std;

using nlohmann::json;

namespace {

class SearchManager_Shard : public ::testing::Test {
  protected:
    void SetUp() override{

    };

    void TearDown() override{};
};

TEST_F(SearchManager_Shard, SearchTopkTeatures) {}

} // namespace
