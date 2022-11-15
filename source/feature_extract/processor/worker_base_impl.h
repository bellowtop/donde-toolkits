#pragma once

#include "Poco/Event.h"
#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"
#include "Poco/Runnable.h"
#include "Poco/Thread.h"
#include "Poco/ThreadPool.h"
#include "donde/definitions.h"
#include "donde/feature_extract/worker.h"
#include "donde/message.h"
#include "nlohmann/json.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <iostream>
#include <memory>
#include <type_traits>

using Poco::NotificationQueue;

using namespace Poco;
using namespace std;

using json = nlohmann::json;

namespace donde {

namespace feature_extract {

// dummy worker impl
class WorkerBaseImpl : public Worker {
  public:
    WorkerBaseImpl(std::shared_ptr<MsgChannel> ch) : _channel(ch){};

    RetCode Init(json conf, int id, std::string device_id) override {
        (void)conf;
        (void)id;
        (void)device_id;
        return {};
    };

    std::string GetName() override { return _name; };

    inline void init_log(const std::string& name) {
        try {
            _logger = spdlog::stdout_color_mt(name);
        } catch (const spdlog::spdlog_ex& ex) {
            _logger = spdlog::get(name);
        }
    }

    void run() override{};

  protected:
    std::shared_ptr<MsgChannel> _channel;
    int _id;
    json _conf;
    std::string _name;
    std::string _device_id;

    std::shared_ptr<spdlog::logger> _logger;
};

} // namespace feature_extract
} // namespace donde
