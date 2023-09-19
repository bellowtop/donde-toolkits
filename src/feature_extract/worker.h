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

} // namespace donde_toolkits::feature_extract
