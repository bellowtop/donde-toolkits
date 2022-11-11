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

#include <functional>
#include <iostream>
#include <memory>
#include <type_traits>

using Poco::AutoPtr;
using Poco::Notification;
using Poco::NotificationQueue;

using namespace Poco;
using namespace std;

using json = nlohmann::json;

enum ChanError { OK, ErrFull, ErrEmpty, ErrClosed };

// Loosely size-bound channel.
// loosely means in a multi-thread environment, the channel size may exceed the size a little.
class MsgChannel {
  public:
    MsgChannel(size_t size = 64) : _size(size), _queue(std::make_shared<NotificationQueue>()){};

    ChanError output(Notification::Ptr& msg) {
        if (closed) {
            return ChanError::ErrClosed;
        }

        if (_queue->empty()) {
            return ChanError::ErrEmpty;
        }
        Notification::Ptr pNf(_queue->waitDequeueNotification());
        if (pNf.isNull()) {
            return ChanError::ErrEmpty;
        }

        msg = pNf;
        return ChanError::OK;
    };

    ChanError input(const Notification::Ptr& msg) {
        if (closed) {
            return ChanError::ErrClosed;
        }

        if (_queue->size() > _size) {
            return ChanError::ErrFull;
        }
        _queue->enqueueNotification(msg);
        return ChanError::OK;
    };

    ChanError close() {
        if (closed) {
            return ChanError::OK;
        }
        closed = true;
        _queue->wakeUpAll();

        return ChanError::OK;
    };

    bool is_closed() { return closed; };

  private:
    size_t _size;
    bool closed;
    std::shared_ptr<NotificationQueue> _queue;
};

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
