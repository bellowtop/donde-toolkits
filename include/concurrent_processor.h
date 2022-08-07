#pragma once

#include "Poco/Event.h"
#include "Poco/Logger.h"
#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"
#include "Poco/Runnable.h"
#include "Poco/Thread.h"
#include "Poco/ThreadPool.h"
#include "nlohmann/json.hpp"
#include "types.h"

#include <iostream>
#include <memory>

using Poco::AutoPtr;
using Poco::FastMutex;
using Poco::Notification;
using Poco::NotificationQueue;
using Poco::Runnable;
using Poco::ThreadPool;
using Poco::Logger;

using namespace Poco;
using namespace std;

using json = nlohmann::json;

class Worker : public Runnable {
  public:
    Worker(std::shared_ptr<NotificationQueue> ch) : _channel(ch){};
    virtual RetCode Init(json conf, int id, std::string device_id) = 0;

    virtual void run() = 0;

  protected:
    std::shared_ptr<NotificationQueue> _channel;
    int _id;
    json _conf;
    std::string _name;
    std::string _device_id;
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
    virtual RetCode Init(json cfg) = 0;
    virtual RetCode Process(Value& input, Value& output) = 0;
    virtual RetCode Terminate() = 0;
    virtual std::string GetName() = 0;
};

template <typename T>
class ConcurrentProcessor : public Processor {
  public:
    ConcurrentProcessor(json conf, int concurrency, std::string device_id, Logger& logger);
    ~ConcurrentProcessor();

    RetCode Init(json cfg) override;
    RetCode Process(Value& input, Value& output) override;
    RetCode Terminate() override;

    std::string GetName() override;

  private:
    int _concurrency;
    json _conf;
    std::string _name;
    std::string _device_id;
    ThreadPool _pool;
    Poco::Logger& _logger;
    std::shared_ptr<NotificationQueue> _channel;

    std::vector<std::shared_ptr<T>> _workers;
    void CreateWorkers(int concurrent);
};

//
// Definition of this templating class.
//

template <typename T>
ConcurrentProcessor<T>::ConcurrentProcessor(json conf, int concurrent, std::string device_id, Logger& parent)
    : _concurrency(concurrent),
      _conf(conf),
      _name("concurrent-process-master"),
      _device_id(device_id),
      _pool(Poco::ThreadPool(1, concurrent)),
      _logger(Logger::get(parent.name() + "." + _name)),
      _channel(std::make_shared<NotificationQueue>()), // create a channel
      _workers(0) {
    CreateWorkers(concurrent);
};

template <typename T>
ConcurrentProcessor<T>::~ConcurrentProcessor() {
    // _channel.reset();
    _workers.clear();
};

template <typename T>
void ConcurrentProcessor<T>::CreateWorkers(int concurrent) {
    for (int i = 0; i < concurrent; i++) {
        _workers.push_back(std::make_shared<T>(T(_channel, _logger)));
    }
    std::cout << _name << " create workers: " << _workers.size() << std::endl;
}

template <typename T>
RetCode ConcurrentProcessor<T>::Init(json conf) {
    // json v = config["id"];
    // Value *value = (Value *)(std::uintptr_t)v;

    // std::cout << "value->type: " << value->type << std::endl;
    // std::cout << "value->content: " << value->content << std::endl;

    int id = 0;
    for (auto it = _workers.begin(); it != _workers.end(); it++) {
        RetCode ret = (*it)->Init(conf, id++, _device_id);
        if (ret != RET_OK) {
            // Logger::Errorf("failed to init worker");
            return ret;
        }
    }

    // start workers
    for (auto it = _workers.begin(); it != _workers.end(); it++) {
        _pool.start(*(it->get()));
    }

    return RET_OK;
}

template <typename T>
RetCode ConcurrentProcessor<T>::Process(Value& input, Value& output) {
    _logger.information("input.valueType : %d, valuePtr: %d\n", input.valueType, input.valuePtr);

    WorkMessage::Ptr msg = WorkMessage::Ptr(new WorkMessage(input, false));
    _channel->enqueueNotification(msg);

    msg->waitResponse();

    Value resp = msg->getResponse();
    output = resp;

    _logger.information("output.valueType : %d, valuePtr: %d\n", output.valueType, output.valuePtr);

    return RET_OK;
}

template <typename T>
RetCode ConcurrentProcessor<T>::Terminate() {
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

template <typename T>
std::string ConcurrentProcessor<T>::GetName() {
    return _name;
}
