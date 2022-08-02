#include "Poco/AutoPtr.h"
#include "Poco/Logger.h"
#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"
#include "concurrent_processor.h"
#include "detector_worker.h"
#include "types.h"
#include "utils.h"

#include "openvino/openvino.hpp"

#include <filesystem>
#include <iostream>
#include <string>

using Poco::Logger;
using Poco::Notification;
using Poco::NotificationQueue;

using namespace Poco;

DetectorWorker::DetectorWorker(std::shared_ptr<NotificationQueue> ch) : Worker(ch) {}

DetectorWorker::~DetectorWorker() {
    // _channel.reset();
}

/**

   conf:
   {
     "model": "../models/face-detection-adas-0001.xml"
   }

 */

RetCode DetectorWorker::Init(json conf, int i, std::string device_id) {
    _id = i;
    _name = "detector-worker-" + std::to_string(i);
    _device_id = device_id;
    _conf = conf;

    std::string model_path = conf["model"];

    std::cout << "loading model: " << model_path << std::endl;
    std::cout << "abs path: " << std::filesystem::canonical(model_path) << std::endl;

    ov::Core core;
    std::shared_ptr<ov::Model> model = core.read_model(model_path);
    printInputAndOutputsInfo(*model);

    OPENVINO_ASSERT(model->inputs().size() == 1, "Sample supports models with 1 input only");
    OPENVINO_ASSERT(model->outputs().size() == 1, "Sample supports models with 1 output only");

    std::string warmup_image = "";

    return RET_OK;
}

void DetectorWorker::run() {
    for (;;) {
        Notification::Ptr pNf(_channel->waitDequeueNotification());

        if (pNf) {
            WorkMessage::Ptr msg = pNf.cast<WorkMessage>();
            if (msg) {
                if (msg->isQuitMessage()) {
                    break;
                }
                Value input = msg->getRequest();
                DetectResult result;
                Value output{ValueDetectResult, (void*)&result};
                msg->setResponse(output);
            }
        } else {
            break;
        }
    }

}
