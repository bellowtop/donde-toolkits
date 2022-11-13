#include "search/impl/simple_driver.h"

#include "search/definitions.h"
#include "types.h"
#include "utils.h"

#include <filesystem>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <ostream>

using namespace std;

using nlohmann::json;

TEST(SearchManager, SimpleDriver) {
    const int feature_count = 2;
    const int dim = 512;

    std::vector<search::FeatureDbItem> fts;
    for (int i = 0; i < feature_count; i++) {
        auto ft = gen_feature_dim<dim>();
        std::map<string, string> meta;
        fts.push_back({
            .feature = ft,
            .metadata = meta,
        });
    }

    std::cout << "in simple_driver.cpp[test] feature length: " << fts.size() << std::endl;

    EXPECT_EQ("aa", "aa");
};

TEST(SearchManager, DBCreateGet) {
    // setup
    search::SimpleDriver store("/tmp/test_store");

    search::DBItem db1{
        .name = "test-db1",
        .description = "this is a test db",
        .capacity = 1024,
    };

    std::string db_id = store.CreateDB(db1);
    search::DBItem got = store.FindDB(db_id);

    EXPECT_EQ(db_id, got.db_id);
    EXPECT_EQ(db1.name, got.name);
    EXPECT_EQ(db1.description, got.description);
    EXPECT_EQ(db1.capacity, got.capacity);

    // teardown
    std::filesystem::remove_all("/tmp/test_store/");
}

TEST(SearchManager, DBCreateDelete) {
    // setup
    search::SimpleDriver store("/tmp/test_store");

    search::DBItem db1{
        .name = "test-db1",
        .description = "this is a test db",
        .capacity = 1024,
    };

    std::string db_id = store.CreateDB(db1);
    search::DBItem got = store.FindDB(db_id);
    EXPECT_EQ(db_id, got.db_id);

    store.DeleteDB(db_id);
    search::DBItem got2 = store.FindDB(db_id);
    EXPECT_EQ(got2.db_id, "");
    EXPECT_EQ(got2.name, "");
    EXPECT_EQ(got2.capacity, 0);

    // teardown
    std::filesystem::remove_all("/tmp/test_store/");
}

TEST(SearchManager, DBlist) {
    // setup
    search::SimpleDriver store("/tmp/test_store");

    search::DBItem db1{
        .name = "test-db1",
        .description = "this is a test db1",
        .capacity = 1024,
    };
    search::DBItem db2{
        .name = "test-db2",
        .description = "this is a test db2",
        .capacity = 1024 * 1024,
    };
    search::DBItem db3{
        .name = "test-db3",
        .description = "this is a test db3",
        .capacity = 1024 * 1024 * 1024,
    };

    std::string db_id1 = store.CreateDB(db1);
    std::string db_id2 = store.CreateDB(db2);
    std::string db_id3 = store.CreateDB(db3);

    std::vector<search::DBItem> got = store.ListDBs();
    EXPECT_EQ(got.size(), 3);

    store.DeleteDB(db_id1);
    store.DeleteDB(db_id2);
    store.DeleteDB(db_id3);

    std::vector<search::DBItem> got2 = store.ListDBs();
    EXPECT_EQ(got2.size(), 0);

    // teardown
    std::filesystem::remove_all("/tmp/test_store/");
}

TEST(SearchManager, SimpleDriverCanManageFeatures) {
    const int feature_count = 2;
    const int dim = 512;

    std::vector<search::FeatureDbItem> fts;
    for (int i = 0; i < feature_count; i++) {
        auto ft = gen_feature_dim<dim>();
        std::map<string, string> meta;
        fts.push_back({
            .feature = ft,
            .metadata = meta,
        });
    }
}

TEST(SearchManager, FeatureManage) {

    // setup
    search::SimpleDriver store("/tmp/test_store");

    search::DBItem db1{
        .name = "test-db1",
        .description = "this is a test db1",
        .capacity = 1024,
    };
    std::string db_id1 = store.CreateDB(db1);

    // teardown
    std::filesystem::remove_all("/tmp/test_store/");
}

TEST(SearchManager, featureCrud) {
    const int feature_count = 2;
    const int dim = 512;

    std::vector<search::FeatureDbItem> fts;
    for (int i = 0; i < feature_count; i++) {
        auto ft = gen_feature_dim<dim>();
        std::map<string, string> meta;
        fts.push_back({
            .feature = ft,
            .metadata = meta,
        });
    }

    // setup
    search::SimpleDriver store("/tmp/test_store");

    search::DBItem db1{
        .name = "test-db1",
        .description = "this is a test db1",
        .capacity = 1024,
    };
    std::string db_id1 = store.CreateDB(db1);

    std::vector<std::string> feature_ids = store.AddFeatures(db_id1, fts);
    EXPECT_EQ(feature_ids.size(), feature_count);

    // file exists
    for (auto& id : feature_ids) {
        std::string p1("/tmp/test_store/data/" + id + ".ft");
        EXPECT_EQ(std::filesystem::exists(p1), true);
    }

    search::PageData<search::FeatureDbItemList> listed = store.ListFeatures(db_id1, 0, 10);
    std::vector<std::string> listed_feature_ids = search::convert_to_feature_ids(listed.data);
    EXPECT_EQ(listed_feature_ids.size(), feature_ids.size());
    for (size_t i = 0; i < listed_feature_ids.size(); i++) {
        EXPECT_EQ(listed_feature_ids[i], feature_ids[i]);
    }

    std::vector<Feature> loaded_features = store.LoadFeatures(db_id1, listed_feature_ids);
    for (size_t j = 0; j < loaded_features.size(); j++) {
        for (size_t i = 0; i < dim; i++) {
            EXPECT_EQ(fts[j].feature.raw[i], loaded_features[j].raw[i]);
        }
    }

    store.RemoveFeatures(db_id1, feature_ids);

    listed = store.ListFeatures(db_id1, 0, 10);

    // no features in db.
    EXPECT_EQ(listed.data.size(), 0);

    // file removed
    for (auto& id : feature_ids) {
        std::string p1("/tmp/test_store/data/" + id + ".ft");
        EXPECT_EQ(std::filesystem::exists(p1), false);
    }

    // teardown
    std::filesystem::remove_all("/tmp/test_store/");
}

TEST(SearchManager, FeaturesMetaDbSupportPageListing) {
    const int feature_count = 105;
    const int dim = 512;

    std::vector<search::FeatureDbItem> fts;
    for (int i = 0; i < feature_count; i++) {
        auto ft = gen_feature_dim<dim>();
        std::map<string, string> meta;
        fts.push_back({
            .feature = ft,
            .metadata = meta,
        });
    }

    // setup
    search::SimpleDriver store("/tmp/test_store");

    search::DBItem db1{
        .name = "test-db1",
        .description = "this is a test db1",
        .capacity = 1024,
    };
    std::string db_id1 = store.CreateDB(db1);

    // cleanup db file
    std::filesystem::remove_all("/tmp/test_store/");

    EXPECT_EQ("aa", "aa");
}

TEST(SearchManager, Paging) {
    const int feature_count = 105;
    const int dim = 512;

    std::vector<search::FeatureDbItem> fts;
    for (int i = 0; i < feature_count; i++) {
        auto ft = gen_feature_dim<dim>();
        std::map<string, string> meta;
        fts.push_back({
            .feature = ft,
            .metadata = meta,
        });
    }

    // setup
    search::SimpleDriver store("/tmp/test_store");

    search::DBItem db1{
        .name = "test-db1",
        .description = "this is a test db1",
        .capacity = 1024,
    };
    std::string db_id1 = store.CreateDB(db1);

    std::vector<std::string> feature_ids = store.AddFeatures(db_id1, fts);
    EXPECT_EQ(feature_ids.size(), feature_count);

    int page = 0;
    int perPage = 10;
    search::PageData<search::FeatureDbItemList> listed = store.ListFeatures(db_id1, page, perPage);
    EXPECT_EQ(listed.data.size(), perPage);
    EXPECT_EQ(listed.page, page);
    EXPECT_EQ(listed.perPage, perPage);
    EXPECT_EQ(listed.totalPage, (feature_count + perPage - 1) / perPage);

    page = 1;

    listed = store.ListFeatures(db_id1, page, perPage);
    EXPECT_EQ(listed.data.size(), perPage);
    EXPECT_EQ(listed.page, page);
    EXPECT_EQ(listed.perPage, perPage);
    EXPECT_EQ(listed.totalPage, (feature_count + perPage - 1) / perPage);

    // large page, // total 105 fts, page 10(the last page) only get 5
    page = 10;

    listed = store.ListFeatures(db_id1, page, perPage);
    EXPECT_EQ(listed.data.size(), 5);
    EXPECT_EQ(listed.page, page);
    EXPECT_EQ(listed.perPage, perPage);
    EXPECT_EQ(listed.totalPage, (feature_count + perPage - 1) / perPage);

    page = 11;

    listed = store.ListFeatures(db_id1, page, perPage);
    EXPECT_EQ(listed.data.size(), 0);
    EXPECT_EQ(listed.page, page);
    EXPECT_EQ(listed.perPage, perPage);
    EXPECT_EQ(listed.totalPage, (feature_count + perPage - 1) / perPage);

    page = 100;

    listed = store.ListFeatures(db_id1, page, perPage);
    EXPECT_EQ(listed.data.size(), 0);
    EXPECT_EQ(listed.page, page);
    EXPECT_EQ(listed.perPage, perPage);
    EXPECT_EQ(listed.totalPage, (feature_count + perPage - 1) / perPage);

    // large perPage
    page = 0;
    perPage = 100;

    listed = store.ListFeatures(db_id1, page, perPage);
    EXPECT_EQ(listed.data.size(), 100);
    EXPECT_EQ(listed.page, page);
    EXPECT_EQ(listed.perPage, perPage);
    EXPECT_EQ(listed.totalPage, (feature_count + perPage - 1) / perPage);

    page = 1;
    perPage = 100;

    listed = store.ListFeatures(db_id1, page, perPage);
    EXPECT_EQ(listed.data.size(), 5);
    EXPECT_EQ(listed.page, page);
    EXPECT_EQ(listed.perPage, perPage);
    EXPECT_EQ(listed.totalPage, (feature_count + perPage - 1) / perPage);

    // even larger perPage
    page = 0;
    perPage = 200;

    listed = store.ListFeatures(db_id1, page, perPage);
    EXPECT_EQ(listed.data.size(), 105);
    EXPECT_EQ(listed.page, page);
    EXPECT_EQ(listed.perPage, perPage);
    EXPECT_EQ(listed.totalPage, (feature_count + perPage - 1) / perPage);

    // cleanup db file
    std::filesystem::remove_all("/tmp/test_store/");
}
