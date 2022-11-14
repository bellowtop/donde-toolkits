#pragma once

#include "definitions.h"
#include "faiss/Index2Layer.h"
#include "nlohmann/json.hpp"

#include <map>
#include <memory>
#include <opencv2/core/hal/interface.h>

using namespace std;

using json = nlohmann::json;

namespace search {

class DBManager {

  public:
    DBManager(const json& config, const json& driver_config);
    ~DBManager() = default;

    RetCode DBNew();

    RetCode DBList();

    RetCode DBGet();

    RetCode DBDelete();
};

} // namespace search
