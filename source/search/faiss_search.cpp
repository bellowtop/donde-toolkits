#include "nlohmann/json.hpp"
#include "search/engine.h"
#include "search/searcher.h"
#include "types.h"

#include <map>

using namespace std;

using json = nlohmann::json;

namespace search {

    FaissSearch::FaissSearch(const json& config) : _config(config){};

} // namespace search
