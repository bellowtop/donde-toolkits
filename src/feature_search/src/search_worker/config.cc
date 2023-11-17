#include "config.h"

#include "nlohmann/json.hpp"

#include <fstream>
#include <iostream>
#include <string>

using json = nlohmann::json;

namespace donde_toolkits ::feature_search ::search_worker {

std::string Config::_config_filepath = "";

void Config::init(const std::string path) { _config_filepath = path; };

Config Config::getInstance() {
    static Config inst;
    return inst;
};

} // namespace donde_toolkits::feature_search::search_worker
