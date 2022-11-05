#pragma once

#include "nlohmann/json.hpp"
#include "types.h"

namespace search {

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
        uint64 capacity;
        uint64 used;
        std::string description;
    };

    struct DBShard {
        std::string db_id;
        std::string shard_id;
    };

    const std::string SEARCH_ENGINE_BRUTE_FORCE = "brute_force";
    const std::string SEARCH_ENGINE_FAISS = "faiss";

    const std::string STORAGE_BACKEND_FILE_SYSTEM = "file_system";
    const std::string STORAGE_BACKEND_CASSANDRA = "cassandra";

    struct FeatureSearchItem {
        Feature target;
        float score;
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

} // namespace search
