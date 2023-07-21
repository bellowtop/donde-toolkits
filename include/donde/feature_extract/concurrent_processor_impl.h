#pragma once

#include "Poco/Event.h"
#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"
#include "Poco/Runnable.h"
#include "Poco/Thread.h"
#include "Poco/ThreadPool.h"
#include "donde/definitions.h"
#include "donde/feature_extract/processor.h"
#include "donde/message.h"
#include "nlohmann/json.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <iostream>
#include <memory>
#include <type_traits>

using Poco::NotificationQueue;
using Poco::ThreadPool;

using namespace Poco;
using namespace std;

using json = nlohmann::json;

namespace donde_toolkits ::feature_extract {

class Worker;

template <typename T, typename U = void>
class ConcurrentProcessorImpl {};

template <typename T>
class ConcurrentProcessorImpl<T, typename std::enable_if_t<std::is_base_of_v<Worker, T>>> final
    : public Processor {
  public:
    ConcurrentProcessorImpl();
    ~ConcurrentProcessorImpl();

    RetCode Init(const json& cfg) override;
    bool IsInited() override;
    RetCode Process(const Value& input, Value& output) override;
    RetCode Terminate() override;

    std::string GetName() override;

  private:
    std::string _name;
    ThreadPool _pool;
    std::shared_ptr<MsgChannel> _channel;
    std::vector<std::shared_ptr<T>> _workers;

    bool _is_inited;
};

//
// Definition of this templating class.
//

template <typename T>
ConcurrentProcessorImpl<
    T, typename std::enable_if_t<std::is_base_of_v<Worker, T>>>::ConcurrentProcessorImpl()
    : _name("concurrent-process-master"),
      _pool(Poco::ThreadPool(1, 1)),            // TODO 1,1?
      _channel(std::make_shared<MsgChannel>()), // create a channel
      _workers(0){};

template <typename T>
ConcurrentProcessorImpl<
    T, typename std::enable_if_t<std::is_base_of_v<Worker, T>>>::~ConcurrentProcessorImpl() {
    // _channel.reset();
    _workers.clear();
};

template <typename T>
RetCode ConcurrentProcessorImpl<T, typename std::enable_if_t<std::is_base_of_v<Worker, T>>>::Init(
    const json& conf) {
    int concurrent = conf["concurrent"];
    std::string device_id = conf["device_id"];

    _pool.addCapacity(concurrent);

    for (int i = 0; i < concurrent; i++) {
        auto worker = std::make_shared<T>(_channel);
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

    _is_inited = true;

    return RET_OK;
}

template <typename T>
bool ConcurrentProcessorImpl<T,
                             typename std::enable_if_t<std::is_base_of_v<Worker, T>>>::IsInited() {
    return _is_inited;
}

template <typename T>
RetCode
ConcurrentProcessorImpl<T, typename std::enable_if_t<std::is_base_of_v<Worker, T>>>::Process(
    const Value& input, Value& output) {
    spdlog::info("input.valueType : {}, valuePtr: {}", format_value_type(input.valueType),
                 input.valuePtr.get());

    WorkMessage<Value>::Ptr msg = WorkMessage<Value>::Ptr(new WorkMessage(input));
    _channel->enqueueNotification(msg);
    Value resp = msg->waitResponse();
    output = resp;

    spdlog::info("output.valueType : {}, valuePtr: {}", format_value_type(output.valueType),
                 output.valuePtr.get());

    return RET_OK;
}

template <typename T>
RetCode
ConcurrentProcessorImpl<T, typename std::enable_if_t<std::is_base_of_v<Worker, T>>>::Terminate() {
    _channel->wakeUpAll();
    _pool.joinAll();

    // Logger::Tracef("controller shutdown all workers");

    return RET_OK;
}

template <typename T>
std::string
ConcurrentProcessorImpl<T, typename std::enable_if_t<std::is_base_of_v<Worker, T>>>::GetName() {
    return _name;
}

} // namespace donde_toolkits::feature_extract
