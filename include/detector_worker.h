#pragma once

#include "Poco/Event.h"
#include "Poco/Logger.h"
#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"
#include "Poco/Runnable.h"
#include "Poco/Thread.h"
#include "Poco/ThreadPool.h"
#include "concurrent_processor.h"
#include "types.h"
#include "nlohmann/json.hpp"

#include "openvino/openvino.hpp"

#include <iostream>
#include <memory>

using Poco::AutoPtr;
using Poco::FastMutex;
using Poco::Logger;
using Poco::Notification;
using Poco::NotificationQueue;
using Poco::Runnable;
using Poco::ThreadPool;

using json = nlohmann::json;

using namespace Poco;

class DetectorWorker : public Worker {
  public:
    DetectorWorker(std::shared_ptr<NotificationQueue> ch);
    ~DetectorWorker();

    RetCode Init(json conf, int id, std::string device_id) override;

    void run() override;

  private:

    RetCode process(cv::Mat& frame, DetectResult& result);

    Logger& _logger = Logger::get("DetectorLogger");

    int _batch_size = 1;
    int _image_width;
    int _image_height;
    int _color_channel = 3;

    std::shared_ptr<ov::Model> _model;
    std::shared_ptr<ov::CompiledModel> _compiled_model;
    std::shared_ptr<ov::InferRequest> _infer_request;


};

// class LandmarksWorker : public Worker {
//   public:
//     LandmarksWorker(std::shared_ptr<NotificationQueue> ch);
//     ~LandmarksWorker();
//
//     RetCode Init(json conf, int id, std::string device_id) override;
//
//     void run() override;
//
//   private:
//     Poco::Logger& _logger;
// };
//
// class AlignerWorker : public Worker {
//   public:
//     AlignerWorker(std::shared_ptr<NotificationQueue> ch);
//     ~AlignerWorker();
//
//     RetCode Init(json conf, int id, std::string device_id) override;
//
//     void run() override;
//
//   private:
//     Poco::Logger& _logger;
// };
//
// class FeatureWorker : public Worker {
//   public:
//     FeatureWorker(std::shared_ptr<NotificationQueue> ch);
//     ~FeatureWorker();
//
//     RetCode Init(json conf, int id, std::string device_id) override;
//
//     void run() override;
//
//   private:
//     Poco::Logger& _logger;
// };
//
