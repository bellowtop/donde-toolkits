#pragma once

#include "Poco/Event.h"
#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"
#include "Poco/Runnable.h"
#include "Poco/Thread.h"
#include "Poco/ThreadPool.h"
#include "concurrent_processor.h"
#include "nlohmann/json.hpp"
#include "openvino/openvino.hpp"
#include "types.h"

#include <iostream>
#include <memory>

using Poco::NotificationQueue;

using json = nlohmann::json;

using namespace Poco;

class DetectorWorker : public Worker {
  public:
    DetectorWorker(std::shared_ptr<NotificationQueue> ch);
    ~DetectorWorker();

    RetCode Init(json conf, int id, std::string device_id) override;

    void run() override;

  private:
    RetCode process(const cv::Mat& frame, DetectResult& result);
    void debugOutputTensor(const ov::Tensor& output);

    int _batch_size = 1;
    int _max_faces;
    int _shape_dim;
    int _image_width;
    int _image_height;
    int _color_channel = 3;

    constexpr static const float _min_confidence = 0.3f;

    std::shared_ptr<ov::CompiledModel> _compiled_model;
    std::shared_ptr<ov::InferRequest> _infer_request;

    std::shared_ptr<spdlog::logger> _logger;
};

class LandmarksWorker : public Worker {
  public:
    LandmarksWorker(std::shared_ptr<NotificationQueue> ch);
    ~LandmarksWorker();

    RetCode Init(json conf, int id, std::string device_id) override;

    void run() override;

  private:
    RetCode process(const DetectResult& detectResult, LandmarksResult& result);
    void debugOutputTensor(const ov::Tensor& output);

    int _batch_size = 1;
    int _landmarks_length;
    int _image_width;
    int _image_height;
    int _color_channel = 3;

    constexpr static const float _min_confidence = 0.3f;

    std::shared_ptr<ov::CompiledModel> _compiled_model;
    std::shared_ptr<ov::InferRequest> _infer_request;

    std::shared_ptr<spdlog::logger> _logger;
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
class AlignerWorker : public Worker {
  public:
    AlignerWorker(std::shared_ptr<NotificationQueue> ch);

    ~AlignerWorker();

    RetCode Init(json conf, int id, std::string device_id) override;

    void run() override;

  private:

    RetCode process(const LandmarksResult& landmarks_result, AlignerResult& result);

    cv::Mat align_face(const cv::Mat& face_image, const std::vector<cv::Point2f>& landmarks);

    std::shared_ptr<spdlog::logger> _logger;
};

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
