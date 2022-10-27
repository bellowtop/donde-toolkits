#include "search/storage.h"
#include "types.h"
#include "utils.h"

#include <iostream>
#include <ostream>

#include <doctest/doctest.h>
#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>

using namespace std;

using nlohmann::json;

template <int size>
Feature gen_feature_dim() {
    std::vector<float> raw(size);
    for (int i = 0; i < size; i++) {
        // 0.000 ~ 0.512
        raw[i] = 0.001 * i;
    }
    return Feature(std::move(raw), "test-model-face", size);
}



TEST_CASE("Feature file can be stored, loaded, and removed.") {

    json conf = R"(
{
		    "path": "/tmp/test_store/"
})"_json;

    const int dim = 512;
    search::FileSystemStorage store(conf);
    store.Init();

    std::vector<Feature> fts;
    for (int i = 0; i < 2; i++) {
        auto ft = gen_feature_dim<dim>();
        fts.push_back(ft);
    }

    std::cout << "in file_system_storage.cpp[test] feature length: "<< fts.size() << std::endl;

    std::vector<std::string> feature_ids = store.AddFeatures(fts);
    CHECK(feature_ids.size() == 2);

    // db file exists
    std::string db_filepath = "/tmp/test_store/meta/sqlite3.db";
    CHECK(std::filesystem::exists(db_filepath) == true);

    // file exists
    for (auto& id : feature_ids) {
        std::string p1("/tmp/test_store/data/" + id + ".ft");
        CHECK(std::filesystem::exists(p1) == true);
    }

    search::PageData<search::FeatureIDList> listed = store.ListFeautreIDs(0, 10);
    CHECK(listed.data.size() == feature_ids.size());
    for (size_t i = 0; i < listed.data.size(); i ++ ) {
        CHECK(listed.data[i] == feature_ids[i]);
    }

    std::vector<Feature> loaded_features = store.LoadFeatures(listed.data);
    for (size_t j = 0; j < loaded_features.size(); j ++) {
        for (size_t i = 0; i < dim; i ++) {
            CHECK(fts[j].raw[i] == loaded_features[j].raw[i]);
        }
    }

    store.RemoveFeatures(feature_ids);
    listed = store.ListFeautreIDs(0, 10);
    // no features in db.
    CHECK(listed.data.size() == 0);

    // file removed
    for (auto& id : feature_ids) {
        std::string p1("/tmp/test_store/data/" + id + ".ft");
        CHECK(std::filesystem::exists(p1) == false);
    }

    // cleanup db file
    std::filesystem::remove_all("/tmp/test_store/");

    CHECK("aa" == "aa");
};


TEST_CASE("Features meta db support page listing.") {

    json conf = R"(
{
		    "path": "/tmp/test_store/"
})"_json;

    const int feature_count = 105;
    const int dim = 512;
    search::FileSystemStorage store(conf);
    store.Init();

    std::vector<Feature> fts;
    for (int i = 0; i < feature_count; i++) {
        auto ft = gen_feature_dim<dim>();
        fts.push_back(ft);
    }

    std::cout << "in file_system_storage.cpp[test] feature length: "<< fts.size() << std::endl;

    std::vector<std::string> feature_ids = store.AddFeatures(fts);
    CHECK(feature_ids.size() == feature_count);

    int page = 0;
    int perPage = 10;
    search::PageData<search::FeatureIDList> listed = store.ListFeautreIDs(page, perPage);
    CHECK(listed.data.size() == perPage);
    CHECK(listed.page == page);
    CHECK(listed.perPage == perPage);
    CHECK(listed.totalPage == (feature_count + perPage - 1) / perPage);

    page++;

    listed = store.ListFeautreIDs(page, perPage);
    CHECK(listed.data.size() == perPage);
    CHECK(listed.page == page);
    CHECK(listed.perPage == perPage);
    CHECK(listed.totalPage == (feature_count + perPage - 1) / perPage);

    page = 100;

    listed = store.ListFeautreIDs(page, perPage);
    CHECK(listed.data.size() == 0);
    CHECK(listed.page == page);
    CHECK(listed.perPage == perPage);
    CHECK(listed.totalPage == (feature_count + perPage - 1) / perPage);

    page = 0;
    perPage = 100;

    listed = store.ListFeautreIDs(page, perPage);
    CHECK(listed.data.size() == 100);
    CHECK(listed.page == page);
    CHECK(listed.perPage == perPage);
    CHECK(listed.totalPage == (feature_count + perPage - 1) / perPage);

    page = 1;
    perPage = 100;

    listed = store.ListFeautreIDs(page, perPage);
    CHECK(listed.data.size() == 5);
    CHECK(listed.page == page);
    CHECK(listed.perPage == perPage);
    CHECK(listed.totalPage == (feature_count + perPage - 1) / perPage);


    page = 0;
    perPage = 200;

    listed = store.ListFeautreIDs(page, perPage);
    CHECK(listed.data.size() == 105);
    CHECK(listed.page == page);
    CHECK(listed.perPage == perPage);
    CHECK(listed.totalPage == (feature_count + perPage - 1) / perPage);


    // cleanup db file
    std::filesystem::remove_all("/tmp/test_store/");

    CHECK("aa" == "aa");
}
