#include "faiss_searcher.h"

#include "nlohmann/json.hpp"

#include <map>

using namespace std;

using json = nlohmann::json;

namespace donde {

namespace feature_search {

namespace search_worker {

FaissSearcher::FaissSearcher(const json& config, Driver& driver)
    : _config(config), _driver(driver){};

}
} // namespace feature_search

} // namespace donde
