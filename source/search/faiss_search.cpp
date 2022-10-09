#pragma once

#include "nlohmann/json.hpp"
#include "search/searcher.h"
#include "search/search_engine.h"
#include "types.h"

#include <map>

using namespace std;

using json = nlohmann::json;

namespace search {

    FaissSearch::FaissSearch(const json& config) : _config(config) {};

} // namespace search
