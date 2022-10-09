#pragma once

#include "nlohmann/json.hpp"
#include "search/searcher.h"
#include "search/storage_backend.h"
#include "types.h"

#include <map>

using namespace std;

using json = nlohmann::json;

namespace search {

    FileSystemBackend::FileSystemBackend(const json& config) : _config(config) {};

    std::vector<uint64> FileSystemBackend::FileSystemBackend::AddFeatures(const std::vector<Feature>& features) {
        return {};
    };

    RetCode FileSystemBackend::FileSystemBackend::RemoveFeatures(const std::vector<uint64>& feature_ids) {
        return RetCode::RET_OK;
    };


} // namespace search
