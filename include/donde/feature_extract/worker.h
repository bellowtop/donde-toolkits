#pragma once

#include "Poco/Runnable.h"
#include "donde/definitions.h"
#include "donde/message.h"
#include "nlohmann/json.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <iostream>
#include <memory>
#include <type_traits>

using namespace std;

using Poco::Runnable;
using json = nlohmann::json;

namespace donde_toolkits::feature_extract {

class Worker : public Runnable {
  public:
    virtual ~Worker() = default;

    virtual RetCode Init(json conf, int id, std::string device_id) = 0;

    virtual std::string GetName() = 0;

    virtual void run() = 0;
};
;

} // namespace donde_toolkits::feature_extract
