#pragma once

#include "Poco/Event.h"
#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"
#include "Poco/Runnable.h"
#include "Poco/Thread.h"
#include "Poco/ThreadPool.h"
#include "nlohmann/json.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include "types.h"

#include <iostream>
#include <memory>
#include <type_traits>

using Poco::AutoPtr;
using Poco::Notification;
using Poco::NotificationQueue;
using Poco::Runnable;
using Poco::ThreadPool;

using namespace Poco;
using namespace std;

using json = nlohmann::json;

class Worker : public Runnable {
  public:
    Worker(std::shared_ptr<NotificationQueue> ch) : _channel(ch){};
    virtual RetCode Init(json conf, int id, std::string device_id) = 0;
    std::string GetName() {return _name;};

    inline void init_log(const std::string& name) {
        try {
            _logger = spdlog::stdout_color_mt(name);
        } catch (const spdlog::spdlog_ex& ex) {
            _logger = spdlog::get(name);
        }
    }

    virtual void run() = 0;

  protected:
    std::shared_ptr<NotificationQueue> _channel;
    int _id;
    json _conf;
    std::string _name;
    std::string _device_id;

    std::shared_ptr<spdlog::logger> _logger;
};

class WorkMessage : public Notification {
  public:
    typedef AutoPtr<WorkMessage> Ptr;

    WorkMessage(Value req, bool quit_flag);
    bool isQuitMessage();

    Value getRequest();

    void setResponse(Value resp);
    Value getResponse();

    void waitResponse();

  private:
    Value _request;
    Value _response;
    bool _quit_flag;
    Poco::Event _evt;
};

class Processor {
  public:
    virtual RetCode Init(const json& cfg) = 0;
    virtual RetCode Process(const Value& input, Value& output) = 0;
    virtual RetCode Terminate() = 0;
    virtual std::string GetName() = 0;
};

template <typename T,
          typename U = std::enable_if_t< std::is_base_of_v<Worker, T> >>
class ConcurrentProcessor final : public Processor {
  public:
    ConcurrentProcessor();
    ~ConcurrentProcessor();

    RetCode Init(const json& cfg) override;
    RetCode Process(const Value& input, Value& output) override;
    RetCode Terminate() override;

    std::string GetName() override;

  private:
    std::string _name;
    ThreadPool _pool;
    std::shared_ptr<NotificationQueue> _channel;
    std::vector<std::shared_ptr<T>> _workers;
};

//
// Definition of this templating class.
//

template <typename T, typename U>
ConcurrentProcessor<T, U>::ConcurrentProcessor()
    : _name("concurrent-process-master"),
      _pool(Poco::ThreadPool(1, 1)),
      _channel(std::make_shared<NotificationQueue>()), // create a channel
      _workers(0){};

template <typename T, typename U>
ConcurrentProcessor<T, U>::~ConcurrentProcessor() {
    // _channel.reset();
    _workers.clear();
};

template <typename T, typename U>
RetCode ConcurrentProcessor<T, U>::Init(const json& conf) {
    int concurrent = conf["concurrent"];
    std::string device_id = conf["device_id"];

    _pool.addCapacity(concurrent);

    for (int i = 0; i < concurrent; i++) {
        auto worker = std::make_shared<T>(T(_channel));
        RetCode ret = worker->Init(conf, i, device_id);
        if (ret != RET_OK) {
            spdlog::error("failed to init worker");
            return ret;
        }
        spdlog::info("add {} worker-{}", worker->GetName(), i);
        _workers.push_back(worker);
    }

    // start workers
    for (auto it = _workers.begin(); it != _workers.end(); it++) {
        _pool.start(*(it->get()));
    }
    spdlog::info("{} concurrency: {}, created {} workers", _name, concurrent, _workers.size());

    return RET_OK;
}

template <typename T, typename U>
RetCode ConcurrentProcessor<T, U>::Process(const Value& input, Value& output) {
    spdlog::info("input.valueType : {}, valuePtr: {}", format(input.valueType),
                 input.valuePtr.get());

    WorkMessage::Ptr msg = WorkMessage::Ptr(new WorkMessage(input, false));
    _channel->enqueueNotification(msg);

    msg->waitResponse();

    Value resp = msg->getResponse();
    output = resp;

    spdlog::info("output.valueType : {}, valuePtr: {}", format(output.valueType),
                 output.valuePtr.get());

    return RET_OK;
}

template <typename T, typename U>
RetCode ConcurrentProcessor<T, U>::Terminate() {
    // enqueue quit message.
    Value empty{};
    for (size_t i = 0; i < _workers.size(); i++) {
        _channel->enqueueNotification(new WorkMessage(empty, true));
    }

    // every messages are processed, including the quit message.
    while (!_channel->empty()) {
        // Logger::Tracef("controller sleep 100 ms");
        Poco::Thread::sleep(100);
    }
    Poco::Thread::sleep(100);

    _channel->wakeUpAll();
    _pool.joinAll();

    // Logger::Tracef("controller shutdown all workers");

    return RET_OK;
}

template <typename T, typename U>
std::string ConcurrentProcessor<T, U>::GetName() {
    return _name;
}
