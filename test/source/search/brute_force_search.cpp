#include "search/engine.h"
#include "search/searcher.h"
#include "search/storage.h"
#include "types.h"
#include "utils.h"

#include <cstdlib>
#include <doctest/doctest.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <ostream>

using namespace std;

using nlohmann::json;

TEST_CASE("Search topk features.") {

    json conf = R"(
{
        "storage": {
		    "path": "/tmp/test_store/"
        }
})"_json;

    const int feature_count = 100;
    const int dim = 512;
    search::BruteForceSearch search(conf);

    std::vector<search::FeatureDbItem> fts;
    for (int i = 0; i < 2; i++) {
        auto ft = gen_feature_dim<dim>();
        std::map<string, string> meta;
        fts.push_back({ft, meta});
    }

    std::cout << "in brute_force_search.cpp[test] feature count: " << feature_count << std::endl;

    std::vector<std::string> feature_ids = search.AddFeatures(fts);
    CHECK(feature_ids.size() == feature_count);

    Feature query{fts[0].feature};
    int topk = 10;
    std::vector<search::FeatureSearchResult> search_result = search.Search(query, topk);

    for (const auto& r : search_result) {
        std::cout << "score: " << r.score << std::endl;
    }

    CHECK(search_result.size() == topk);

    // the most near ft.
    auto t = search_result[0];
    CHECK(t.score > 0.99);

    // query.debugPrint();
    // t.target.debugPrint();

    for (size_t i = 0; i < t.target.raw.size(); i ++) {
        CHECK(t.target.raw[i] == fts[0].feature.raw[i]);
    }

    // cleanup db file
    std::filesystem::remove_all("/tmp/test_store/");

    CHECK("aa" == "aa");
}
