#pragma once

#include "Poco/Event.h"
#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"
#include "Poco/Runnable.h"
#include "Poco/Thread.h"
#include "Poco/ThreadPool.h"
#include "donde/definitions.h"
#include "nlohmann/json.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <chrono>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <type_traits>

using Poco::AutoPtr;
using Poco::Notification;
using Poco::NotificationQueue;

using namespace Poco;
using namespace std;

using json = nlohmann::json;

namespace donde {

enum ChanError { OK, ErrFull, ErrEmpty, ErrClosed };

// Loosely size-bound channel.
// loosely means in a multi-thread environment, the channel size may exceed the size a little.

using MsgChannel = NotificationQueue;

template <typename V>
class WorkMessage : public Notification {
  public:
    using Ptr = AutoPtr<WorkMessage>;

    WorkMessage(V req, bool quit_flag);
    bool isQuitMessage();

    V getRequest();

    V waitResponse();
    void setResponse(V resp);

  private:
    V _request;
    V _response;
    bool _quit_flag;
    Poco::Event _evt;
};

template <typename V>
using WorkMessagePtr = typename WorkMessage<V>::Ptr;

template <typename V>
WorkMessagePtr<V> NewWorkMessage(V req) {
    return WorkMessagePtr<V>(new WorkMessage(req, false));
};

template <typename V>
WorkMessage<V>::WorkMessage(V request, bool quit_flag) : _request(request), _quit_flag(quit_flag) {
    Event _evt(Event::EVENT_AUTORESET);
    _evt.reset();
}
template <typename V>
bool WorkMessage<V>::isQuitMessage() {
    return _quit_flag;
}

template <typename V>
V WorkMessage<V>::waitResponse() {
    _evt.wait();
    return _response;
}

template <typename V>
V WorkMessage<V>::getRequest() {
    return _request;
}
template <typename V>
void WorkMessage<V>::setResponse(V resp) {
    _response = resp;

    _evt.set();
}

} // namespace donde
