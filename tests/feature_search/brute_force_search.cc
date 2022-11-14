#include "definitions.h"
#include "search_manager/brute_force_searcher.h"
#include "search_manager/simple_driver.h"
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

class SearchManager_BruteForceSearch : public ::testing::Test {
  protected:
    void SetUp() override {
        for (int i = 0; i < feature_count; i++) {
            auto ft = gen_feature_dim<512>();
            std::map<string, string> meta{{"keya", "valueb"}};
            fts.push_back(search::FeatureDbItem{
                .feature = ft,
                .metadata = meta,
            });
        }

        store = new search::SimpleDriver("/tmp/test_store");
        search::DBItem db1{
            .name = "test-db1",
            .description = "this is a test db",
            .capacity = 1024,
        };

        db_id = store->CreateDB(db1);
    };

    void TearDown() override {
        // cleanup db file
        std::filesystem::remove_all("/tmp/test_store/");
    };

    search::SimpleDriver* store;
    const int feature_count = 100;
    const int dim = 512;
    std::vector<search::FeatureDbItem> fts;
    std::string db_id;
};

TEST_F(SearchManager_BruteForceSearch, BruteForceSearch) {
    search::BruteForceSearcher search(*store);

    std::vector<std::string> feature_ids = search.AddFeatures(db_id, fts);
    EXPECT_EQ(feature_ids.size(), feature_count);
}

TEST_F(SearchManager_BruteForceSearch, SearchTopkTeatures) {
    search::BruteForceSearcher search(*store);

    // preapre features in db.
    std::vector<std::string> feature_ids = search.AddFeatures(db_id, fts);
    EXPECT_EQ(feature_ids.size(), feature_count);

    // query is the first one. so that we definitely can find matched fts.
    Feature query{fts[0].feature};

    int topk = 10;
    std::vector<search::FeatureSearchItem> search_result = search.SearchFeature(db_id, query, topk);

    for (const auto& r : search_result) {
        std::cout << "score: " << r.score << std::endl;
    }

    EXPECT_EQ(search_result.size(), topk);

    if (int(search_result.size()) == topk) {
        // the most near ft.
        auto t = search_result[0];
        EXPECT_GT(t.score, 0.99);

        // query.debugPrint();
        // t.target.debugPrint();

        for (size_t i = 0; i < t.target.raw.size(); i++) {
            EXPECT_EQ(t.target.raw[i], fts[0].feature.raw[i]);
        }
    }
}

} // namespace
