#pragma once

#include "nlohmann/json.hpp"
#include "search/searcher.h"
#include "search/storage_backend.h"
#include "types.h"

#include <map>

using namespace std;

using json = nlohmann::json;

namespace search {

    FileSystemBackend::FileSystemBackend(const json& config) : StorageBackend(config){};

} // namespace search
