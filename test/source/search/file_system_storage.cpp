#include "search/storage.h"
#include "types.h"
#include "utils.h"

#include <doctest/doctest.h>
#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>

using namespace std;

using nlohmann::json;

template <int size>
inline Feature gen_feature_dim() {
    std::vector<float> raw(size);
    for (int i = 0; i < size; i++) {
        raw[i] = 0.1;
    }
    return Feature(std::move(raw));
}

TEST_CASE("Feature file can be stored and deleted.") {

    json conf = R"(
{
		"storage": {
		    "path": "/tmp/test_store/"
		}
})"_json;

    search::FileSystemStorage store(conf);
    std::vector<Feature> fts;
    for (int i = 0; i < 2; i++) {
        auto ft = gen_feature_dim<512>();
        fts.push_back(ft);
    }

    std::vector<std::string> feature_ids = store.AddFeatures(fts);
    CHECK(feature_ids.size() == 2);

    // file exists
    for (auto& id : feature_ids) {
        std::string p1("/tmp/test_store/" + id);
        CHECK(std::filesystem::exists(p1) == true);
    }

    store.RemoveFeatures(feature_ids);

    // file removed
    for (auto& id : feature_ids) {
        std::string p1("/tmp/test_store/" + id);
        CHECK(std::filesystem::exists(p1) == false);
    }

    CHECK("aa" == "aa");
};
