#pragma once

#include "nlohmann/json.hpp"
#include "search/search_engine.h"
#include "search/searcher.h"
#include "types.h"

#include <map>

using namespace std;

using json = nlohmann::json;

namespace search {

    BruteForceSearch::BruteForceSearch(const json& config) : _config(config) {}

} // namespace search
