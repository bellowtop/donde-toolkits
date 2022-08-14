#include "Poco/AutoPtr.h"
#include "Poco/Logger.h"
#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"
#include "concurrent_processor.h"
#include "types.h"

#include <iostream>
#include <string>

using Poco::Notification;
using Poco::NotificationQueue;

using namespace Poco;

/*
DetectorWorker::DetectorWorker(std::shared_ptr<NotificationQueue> ch)
    : _channel(ch) {}

DetectorWorker::~DetectorWorker() {
    // _channel.reset();
}

RetCode DetectorWorker::Init(json conf, int i, std::string device_id) {
    _id = i;
    _name = "detector-worker-" + std::to_string(i);
    _logger = Logger::get(_name);
    _device_id = device_id;
    _conf = conf;

    return RET_OK;
}

void DetectorWorker::run() {

}

*/
