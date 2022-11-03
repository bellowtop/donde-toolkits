#include "search/impl/simple_driver.h"

#include "search/definitions.h"
#include "types.h"
#include "utils.h"

#include <doctest/doctest.h>
#include <filesystem>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <ostream>

using namespace std;

using nlohmann::json;

TEST_CASE("SimpleDriver: can manage DB.") {
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

    SUBCASE("DB create/get") {
        // setup
        search::SimpleDriver store("/tmp/test_store");

        search::DBItem db1{
            .name = "test-db1",
            .description = "this is a test db",
            .capacity = 1024,
        };

        std::string db_id = store.CreateDB(db1);
        search::DBItem got = store.FindDB(db_id);

        CHECK(db_id == got.db_id);
        CHECK(db1.name == got.name);
        CHECK(db1.description == got.description);
        CHECK(db1.capacity == got.capacity);

        // teardown
        std::filesystem::remove_all("/tmp/test_store/");
    }

    SUBCASE("DB create/delete") {
        // setup
        search::SimpleDriver store("/tmp/test_store");

        search::DBItem db1{
            .name = "test-db1",
            .description = "this is a test db",
            .capacity = 1024,
        };

        std::string db_id = store.CreateDB(db1);
        search::DBItem got = store.FindDB(db_id);
        CHECK(db_id == got.db_id);

        store.DeleteDB(db_id);
        search::DBItem got2 = store.FindDB(db_id);
        CHECK(got2.db_id == "");
        CHECK(got2.name == "");
        CHECK(got2.capacity == 0);

        // teardown
        std::filesystem::remove_all("/tmp/test_store/");
    }

    SUBCASE("DB list") {
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
        CHECK(got.size() == 3);

        store.DeleteDB(db_id1);
        store.DeleteDB(db_id2);
        store.DeleteDB(db_id3);

        std::vector<search::DBItem> got2 = store.ListDBs();
        CHECK(got.size() == 0);

        // teardown
        std::filesystem::remove_all("/tmp/test_store/");
    }

    CHECK("aa" == "aa");
};

TEST_CASE("SimpleDriver: can manage features.") {
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

    SUBCASE("Feature manage") {

        // setup
        search::SimpleDriver store("/tmp/test_store");

        search::DBItem db1{
            .name = "test-db1",
            .description = "this is a test db1",
            .capacity = 1024,
        };
        std::string db_id1 = store.CreateDB(db1);

        SUBCASE("feature crud") {
            std::vector<std::string> feature_ids = store.AddFeatures(db_id1, fts);
            CHECK(feature_ids.size() == feature_count);

            // file exists
            for (auto& id : feature_ids) {
                std::string p1("/tmp/test_store/data/" + id + ".ft");
                CHECK(std::filesystem::exists(p1) == true);
            }

            search::PageData<search::FeatureDbItemList> listed = store.ListFeatures(db_id1, 0, 10);
            std::vector<std::string> listed_feature_ids
                = search::convert_to_feature_ids(listed.data);
            CHECK(listed_feature_ids.size() == feature_ids.size());
            for (size_t i = 0; i < listed_feature_ids.size(); i++) {
                CHECK(listed_feature_ids[i] == feature_ids[i]);
            }

            std::vector<Feature> loaded_features = store.LoadFeatures(db_id1, listed_feature_ids);
            for (size_t j = 0; j < loaded_features.size(); j++) {
                for (size_t i = 0; i < dim; i++) {
                    CHECK(fts[j].feature.raw[i] == loaded_features[j].raw[i]);
                }
            }

            store.RemoveFeatures(db_id1, feature_ids);

            listed = store.ListFeatures(db_id1, 0, 10);

            // no features in db.
            CHECK(listed.data.size() == 0);

            // file removed
            for (auto& id : feature_ids) {
                std::string p1("/tmp/test_store/data/" + id + ".ft");
                CHECK(std::filesystem::exists(p1) == false);
            }
        }

        // teardown
        std::filesystem::remove_all("/tmp/test_store/");
    }

    SUBCASE("Features meta db support page listing.") {
        // setup
        search::SimpleDriver store("/tmp/test_store");

        search::DBItem db1{
            .name = "test-db1",
            .description = "this is a test db1",
            .capacity = 1024,
        };
        std::string db_id1 = store.CreateDB(db1);

        SUBCASE("") {
            std::vector<std::string> feature_ids = store.AddFeatures(db_id1, fts);
            CHECK(feature_ids.size() == feature_count);

            int page = 0;
            int perPage = 10;
            search::PageData<search::FeatureDbItemList> listed
                = store.ListFeatures(db_id1, page, perPage);
            CHECK(listed.data.size() == perPage);
            CHECK(listed.page == page);
            CHECK(listed.perPage == perPage);
            CHECK(listed.totalPage == (feature_count + perPage - 1) / perPage);

            page = 1;

            listed = store.ListFeatures(db_id1, page, perPage);
            CHECK(listed.data.size() == perPage);
            CHECK(listed.page == page);
            CHECK(listed.perPage == perPage);
            CHECK(listed.totalPage == (feature_count + perPage - 1) / perPage);

            // large page
            page = 10;

            listed = store.ListFeatures(db_id1, page, perPage);
            CHECK(listed.data.size() == 5);
            CHECK(listed.page == page);
            CHECK(listed.perPage == perPage);
            CHECK(listed.totalPage == (feature_count + perPage - 1) / perPage);

            page = 11;

            listed = store.ListFeatures(db_id1, page, perPage);
            CHECK(listed.data.size() == 0);
            CHECK(listed.page == page);
            CHECK(listed.perPage == perPage);
            CHECK(listed.totalPage == (feature_count + perPage - 1) / perPage);

            page = 100;

            listed = store.ListFeatures(db_id1, page, perPage);
            CHECK(listed.data.size() == 0);
            CHECK(listed.page == page);
            CHECK(listed.perPage == perPage);
            CHECK(listed.totalPage == (feature_count + perPage - 1) / perPage);

            // large perPage
            page = 0;
            perPage = 100;

            listed = store.ListFeatures(db_id1, page, perPage);
            CHECK(listed.data.size() == 100);
            CHECK(listed.page == page);
            CHECK(listed.perPage == perPage);
            CHECK(listed.totalPage == (feature_count + perPage - 1) / perPage);

            page = 1;
            perPage = 100;

            listed = store.ListFeatures(db_id1, page, perPage);
            CHECK(listed.data.size() == 5);
            CHECK(listed.page == page);
            CHECK(listed.perPage == perPage);
            CHECK(listed.totalPage == (feature_count + perPage - 1) / perPage);

            // even larger perPage
            page = 0;
            perPage = 200;

            listed = store.ListFeatures(db_id1, page, perPage);
            CHECK(listed.data.size() == 105);
            CHECK(listed.page == page);
            CHECK(listed.perPage == perPage);
            CHECK(listed.totalPage == (feature_count + perPage - 1) / perPage);
        }

        // cleanup db file
        std::filesystem::remove_all("/tmp/test_store/");

        CHECK("aa" == "aa");
    }
}
