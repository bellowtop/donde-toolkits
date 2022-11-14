#include "donde/feature_search/api.h"
#include "donde/feature_search/definitions.h"
#include "donde/utils.h"
#include "source/feature_search/search_manager/shard_impl.h"

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
