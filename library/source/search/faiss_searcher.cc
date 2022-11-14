#include "faiss_searcher.h"

#include "nlohmann/json.hpp"

#include <map>

using namespace std;

using json = nlohmann::json;

namespace search {

FaissSearcher::FaissSearcher(const json& config, Driver& driver)
    : _config(config), _driver(driver){};

} // namespace search
