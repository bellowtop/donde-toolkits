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
    void giveReceipt();

    void setResponse(V resp, bool wait_receipt = false, int wait_receipt_ms = 1000);

  private:
    V _request;
    V _response;

    bool _quit_flag;

    // flag for response is ready
    Poco::Event _evt_response;
    // flag the response is consumed.
    Poco::Event _evt_receipt;
};

template <typename V>
using WorkMessagePtr = typename WorkMessage<V>::Ptr;

template <typename V>
WorkMessagePtr<V> NewWorkMessage(V req) {
    return WorkMessagePtr<V>(new WorkMessage(req, false));
};

template <typename V>
WorkMessage<V>::WorkMessage(V request, bool quit_flag) : _request(request), _quit_flag(quit_flag) {}

template <typename V>
bool WorkMessage<V>::isQuitMessage() {
    return _quit_flag;
}

template <typename V>
V WorkMessage<V>::waitResponse() {
    _evt_response.wait();
    return _response;
}

template <typename V>
void WorkMessage<V>::giveReceipt() {
    _evt_receipt.set();
}

template <typename V>
V WorkMessage<V>::getRequest() {
    return _request;
}
template <typename V>
void WorkMessage<V>::setResponse(V resp, bool wait_receipt, int wait_receipt_ms) {
    _response = resp;

    _evt_response.set();
    if (wait_receipt) {
        // wait timeout, in case of infinite blocking.
        _evt_receipt.wait(wait_receipt_ms);
    }
}

} // namespace donde
