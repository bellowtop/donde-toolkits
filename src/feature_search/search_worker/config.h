#pragma once

#include "nlohmann/json.hpp"

#include <fstream>
#include <iostream>
#include <string>

using json = nlohmann::json;

namespace donde_toolkits {
namespace feature_search {
namespace search_worker {

class Config {
  public:
    inline std::string get_log_level() { return _log_level; };
    inline std::string get_device_id() { return _device_id; };
    inline json get_json_config() { return _json_config; };
    inline json get_searcher_config() { return _search_manager_config; };

    static void init(const std::string path);
    static Config getInstance();

  private:
    Config() {
        assert(_config_filepath.size() != 0);

        // load json from filepath;
        std::ifstream f(_config_filepath);
        std::cout << "_config_filepath:" << _config_filepath << std::endl;
        _json_config = json::parse(f);
        _search_manager_config = _json_config["searcher"];
    };

    static std::string _config_filepath;
    std::string _log_level;
    std::string _device_id;
    json _json_config;
    json _search_manager_config;
};

} // namespace search_worker
} // namespace feature_search
} // namespace donde_toolkits
