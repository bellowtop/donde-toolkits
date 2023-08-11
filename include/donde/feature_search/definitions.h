#pragma once

#include "donde/definitions.h"
#include "nlohmann/json.hpp"

#include <opencv2/core/hal/interface.h>

namespace donde_toolkits {

namespace feature_search {

template <typename T>
struct PageData {
    uint64 page;
    uint64 perPage;
    uint64 totalPage;
    T data;

    PageData() {}

    PageData(uint64 _page, uint64 _perPage, uint64 _totalPage, T _data)
        : page(_page), perPage(_perPage), totalPage(_totalPage), data(_data) {}

    PageData& operator=(const PageData& lhs) {
        page = lhs.page;
        perPage = lhs.perPage;
        totalPage = lhs.totalPage;
        data = lhs.data;
        return *this;
    };
};

struct DBItem {
    std::string db_id;
    std::string name;
    uint64 size;
    std::string description;
};

struct WorkerItem {
    std::string worker_id;
    std::string address;
    uint64 max_capacity;
};

const size_t DEFAULT_SHARD_CAPACITY = 1024 * 1024 * 1024;

struct DBShard {
    std::string db_id;
    std::string shard_id;
    std::string worker_id;
    uint64 capacity;
    uint64 used;
    bool is_closed;
};

using DBItemPtr = shared_ptr<DBItem>;

const std::string SEARCH_ENGINE_BRUTE_FORCE = "brute_force";
const std::string SEARCH_ENGINE_FAISS = "faiss";

const std::string SEARCH_DRIVER_SIMPLE = "simple";
const std::string SEARCH_DRIVER_CASSANDRA = "cassandra";

struct FeatureSearchItem {
    FeatureSearchItem(Feature&& t, float s) : target(t), score(s){};
    FeatureSearchItem(const Feature& t, float s) : target(t), score(s){};
    Feature target;
    float score;
};

struct FeatureSearchComp {
    bool operator()(const FeatureSearchItem& lhs, const FeatureSearchItem& rhs) {
        return lhs.score > rhs.score;
    };
};

struct FeatureDbItem {
    std::string feature_id;
    Feature feature;
    std::map<string, string> metadata;
};

using FeatureDbItemList = std::vector<FeatureDbItem>;

inline std::vector<std::string> convert_to_feature_ids(const FeatureDbItemList& lst) {
    std::vector<std::string> ret;
    for (const auto& item : lst) {
        ret.push_back(item.feature_id);
    }
    return ret;
};

} // namespace feature_search

} // namespace donde_toolkits
