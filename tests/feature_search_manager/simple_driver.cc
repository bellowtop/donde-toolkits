#include "source/feature_search/simple_driver.h"

#include "donde/definitions.h"
#include "donde/feature_search/definitions.h"
#include "donde/utils.h"

#include <filesystem>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <ostream>

using namespace std;

using nlohmann::json;

using donde::Feature;
using donde::gen_feature_dim;

using donde::feature_search::DBItem;
using donde::feature_search::FeatureDbItem;
using donde::feature_search::FeatureDbItemList;
using donde::feature_search::SimpleDriver;
;
using donde::feature_search::convert_to_feature_ids;
using donde::feature_search::PageData;

namespace {

class SearchManager_SimpleDriver : public ::testing::Test {
  protected:
    void SetUp() override {

        fts = generate_features(100);

        store = new SimpleDriver("/tmp/test_store");

        db1 = DBItem{
            .name = "test-db1",
            .description = "this is a test db",
            .capacity = 1024,
        };

        db_id = store->CreateDB(db1);
    };

    void TearDown() override {
        // teardown
        std::filesystem::remove_all("/tmp/test_store/");
    };

    std::vector<FeatureDbItem> generate_features(int feature_count) {
        std::vector<FeatureDbItem> ret;
        for (int i = 0; i < feature_count; i++) {
            auto ft = gen_feature_dim<512>();
            std::map<string, string> meta{{"keya", "valueb"}};
            ret.push_back(FeatureDbItem{
                .feature = ft,
                .metadata = meta,
            });
        }
        return ret;
    };

    SimpleDriver* store;
    DBItem db1;
    int feature_count = 100;
    const int dim = 512;
    std::vector<FeatureDbItem> fts;
    std::string db_id;
};

TEST_F(SearchManager_SimpleDriver, DBCreateGet) {
    DBItem got = store->FindDB(db_id);

    EXPECT_EQ(db_id, got.db_id);
    EXPECT_EQ(db1.name, got.name);
    EXPECT_EQ(db1.description, got.description);
    EXPECT_EQ(db1.capacity, got.capacity);
}

TEST_F(SearchManager_SimpleDriver, DBCreateDelete) {
    store->DeleteDB(db_id);

    DBItem got2 = store->FindDB(db_id);
    EXPECT_EQ(got2.db_id, "");
    EXPECT_EQ(got2.name, "");
    EXPECT_EQ(got2.capacity, 0);
}

TEST_F(SearchManager_SimpleDriver, DBlist) {
    store->DeleteDB(db_id);

    DBItem db1{
        .name = "test-db1",
        .description = "this is a test db1",
        .capacity = 1024,
    };
    DBItem db2{
        .name = "test-db2",
        .description = "this is a test db2",
        .capacity = 1024 * 1024,
    };
    DBItem db3{
        .name = "test-db3",
        .description = "this is a test db3",
        .capacity = 1024 * 1024 * 1024,
    };

    std::string db_id1 = store->CreateDB(db1);
    std::string db_id2 = store->CreateDB(db2);
    std::string db_id3 = store->CreateDB(db3);

    std::vector<DBItem> got = store->ListDBs();
    EXPECT_EQ(got.size(), 3);

    store->DeleteDB(db_id1);
    store->DeleteDB(db_id2);
    store->DeleteDB(db_id3);

    std::vector<DBItem> got2 = store->ListDBs();
    EXPECT_EQ(got2.size(), 0);
}

TEST_F(SearchManager_SimpleDriver, featureCrud) {
    std::vector<std::string> feature_ids = store->AddFeatures(db_id, fts);
    EXPECT_EQ(feature_ids.size(), feature_count);

    // file exists
    for (auto& id : feature_ids) {
        std::string p1("/tmp/test_store/data/" + id + ".ft");
        EXPECT_EQ(std::filesystem::exists(p1), true);
    }

    PageData<FeatureDbItemList> listed = store->ListFeatures(db_id, 0, 100);
    std::vector<std::string> listed_feature_ids = convert_to_feature_ids(listed.data);
    EXPECT_EQ(listed_feature_ids.size(), feature_ids.size());
    for (size_t i = 0; i < listed_feature_ids.size(); i++) {
        EXPECT_EQ(listed_feature_ids[i], feature_ids[i]);
    }

    std::vector<Feature> loaded_features = store->LoadFeatures(db_id, listed_feature_ids);
    for (size_t j = 0; j < loaded_features.size(); j++) {
        for (size_t i = 0; i < size_t(dim); i++) {
            EXPECT_EQ(fts[j].feature.raw[i], loaded_features[j].raw[i]);
        }
    }

    store->RemoveFeatures(db_id, feature_ids);

    listed = store->ListFeatures(db_id, 0, 10);

    // no features in db.
    EXPECT_EQ(listed.data.size(), 0);

    // file removed
    for (auto& id : feature_ids) {
        std::string p1("/tmp/test_store/data/" + id + ".ft");
        EXPECT_EQ(std::filesystem::exists(p1), false);
    }
}

TEST_F(SearchManager_SimpleDriver, Paging) {
    std::vector<std::string> feature_ids = store->AddFeatures(db_id, fts);
    EXPECT_EQ(feature_ids.size(), feature_count);

    // add 5 more. so we have 105 features in db..
    auto more_fts = generate_features(5);
    store->AddFeatures(db_id, more_fts);
    feature_count += 5;

    int page = 0;
    int perPage = 10;

    PageData<FeatureDbItemList> listed = store->ListFeatures(db_id, page, perPage);
    EXPECT_EQ(listed.data.size(), perPage);
    EXPECT_EQ(listed.page, page);
    EXPECT_EQ(listed.perPage, perPage);
    EXPECT_EQ(listed.totalPage, (feature_count + perPage - 1) / perPage);

    page = 1;

    listed = store->ListFeatures(db_id, page, perPage);
    EXPECT_EQ(listed.data.size(), perPage);
    EXPECT_EQ(listed.page, page);
    EXPECT_EQ(listed.perPage, perPage);
    EXPECT_EQ(listed.totalPage, (feature_count + perPage - 1) / perPage);

    // large page, // total 105 fts, page 10(the last page) only get 5
    page = 10;

    listed = store->ListFeatures(db_id, page, perPage);
    EXPECT_EQ(listed.data.size(), 5);
    EXPECT_EQ(listed.page, page);
    EXPECT_EQ(listed.perPage, perPage);
    EXPECT_EQ(listed.totalPage, (feature_count + perPage - 1) / perPage);

    page = 11;

    listed = store->ListFeatures(db_id, page, perPage);
    EXPECT_EQ(listed.data.size(), 0);
    EXPECT_EQ(listed.page, page);
    EXPECT_EQ(listed.perPage, perPage);
    EXPECT_EQ(listed.totalPage, (feature_count + perPage - 1) / perPage);

    page = 100;

    listed = store->ListFeatures(db_id, page, perPage);
    EXPECT_EQ(listed.data.size(), 0);
    EXPECT_EQ(listed.page, page);
    EXPECT_EQ(listed.perPage, perPage);
    EXPECT_EQ(listed.totalPage, (feature_count + perPage - 1) / perPage);

    // large perPage
    page = 0;
    perPage = 100;

    listed = store->ListFeatures(db_id, page, perPage);
    EXPECT_EQ(listed.data.size(), 100);
    EXPECT_EQ(listed.page, page);
    EXPECT_EQ(listed.perPage, perPage);
    EXPECT_EQ(listed.totalPage, (feature_count + perPage - 1) / perPage);

    page = 1;
    perPage = 100;

    listed = store->ListFeatures(db_id, page, perPage);
    EXPECT_EQ(listed.data.size(), 5);
    EXPECT_EQ(listed.page, page);
    EXPECT_EQ(listed.perPage, perPage);
    EXPECT_EQ(listed.totalPage, (feature_count + perPage - 1) / perPage);

    // even larger perPage
    page = 0;
    perPage = 200;

    listed = store->ListFeatures(db_id, page, perPage);
    EXPECT_EQ(listed.data.size(), 105);
    EXPECT_EQ(listed.page, page);
    EXPECT_EQ(listed.perPage, perPage);
    EXPECT_EQ(listed.totalPage, (feature_count + perPage - 1) / perPage);
}
} // namespace
