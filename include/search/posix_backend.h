#pragma once

#include "nlohmann/json.hpp"
#include "search/searcher.h"
#include "types.h"

#include <map>

using namespace std;

using json = nlohmann::json;

namespace search {

    class PosixBackend : public StorageBackend {};

} // namespace search
