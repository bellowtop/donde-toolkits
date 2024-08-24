#include "Poco/AutoPtr.h"
#include "Poco/NotificationQueue.h"
#include "coreml_worker.h"
#include "donde/definitions.h"
#include "donde/message.h"
#include "donde/utils.h"
#include "objc_wrapper.h"
#include "utils.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <memory>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>
#include <spdlog/common.h>
#include <string>

using Poco::Notification;
using Poco::NotificationQueue;

using namespace Poco;

namespace donde_toolkits ::feature_extract ::coreml_worker {

DetectorWorker::DetectorWorker(std::shared_ptr<MsgChannel> ch) : WorkerBaseImpl{ch} {}

DetectorWorker::~DetectorWorker() {
    if (yolov8model != nullptr) {
        closeModel(yolov8model);
    }
}

RetCode DetectorWorker::Init(json conf, int i, std::string device_id) {
    _name = "detector-worker-" + std::to_string(i);
    init_log(_name);

    _id = i;
    _device_id = device_id;
    _conf = conf;

    std::string model_path = conf["model"];

    _logger->info("loading model: {}", model_path);
    _logger->info("absolute path: {}", std::filesystem::canonical(model_path).string());

    model_abs_path = std::filesystem::canonical(model_path).string();

    yolov8model = loadModel(model_abs_path.c_str());

    if (conf.contains("warmup") && conf["warmup"]) {
        // warmup img
        std::string warmup_image = "./contrib/data/test_image_5_person.jpeg";
        cv::Mat img = cv::imread(warmup_image);

        DetectResult result;
        process(img, result);
    }

    return RET_OK;
}

void DetectorWorker::run() {
    for (;;) {
        // output is a blocking call.
        Notification::Ptr pNf = _channel->waitDequeueNotification();
        if (pNf.isNull()) {
            break;
        }
        WorkMessage<Value>::Ptr msg = pNf.cast<WorkMessage<Value>>();
        Value input = msg->getRequest();
        if (input.valueType != ValueFrame) {
            _logger->error("DetectorWorker input value is not a frame! wrong valueType: {}", int(input.valueType));
            continue;
        }
        std::shared_ptr<Frame> f = std::static_pointer_cast<Frame>(input.valuePtr);

        std::shared_ptr<DetectResult> result = std::make_shared<DetectResult>();
        // acquire! hold a reference to the frame.
        result->frame = f;

        RetCode ret = process(f->image, *result);
        _logger->debug("process ret: {}", int(ret));

        Value output{ValueDetectResult, result};
        msg->setResponse(output);
    }
}

// resize input img, and do inference
RetCode DetectorWorker::process(const cv::Mat& image, DetectResult& result) {

    // alloc output buffer
    // output as 1 × 5 × 8400 3-dimensional array of floats
    float* outFloats = (float*)malloc(sizeof(float) * batch * params * boxes);
    std::cout << "cols: " << image.cols << " rows: " << image.rows << std::endl;
    cv::imwrite("/tmp/aaa.jpg", image);

    auto t1 = std::chrono::steady_clock::now();
    predictWith(yolov8model, image, outFloats);
    auto [t2, used_ms] = now_time_since(t1);
    printf("yolov8 coreml predict use time: %lld ms\n", used_ms.count());

    std::vector<Yolov8DetBox> candidateBoxes;

    for (int i = 0; i < batch; i++) {
        std::cout << "Image " << i << std::endl;
        // for (int j = 0; j < params; j++) {
        for (int k = 0; k < boxes; k++) {
            float confidence = outFloats[i * 4 * boxes + 4 * boxes + k];
            if (confidence > min_confidence) {
                float centerX = outFloats[i * 0 * boxes + 0 * boxes + k];
                float centerY = outFloats[i * 1 * boxes + 1 * boxes + k];
                float width = outFloats[i * 2 * boxes + 2 * boxes + k];
                float height = outFloats[i * 3 * boxes + 3 * boxes + k];
                auto box = Yolov8DetBox{centerX, centerY, width, height, confidence};
                candidateBoxes.push_back(box);
            }
        }
        // }
    }

    float factorX = image.cols * 1.0f / image_width;
    float factorY = image.rows * 1.0f / image_height;

    auto keep = nonMaxSuppression(candidateBoxes, 0.8);

    std::vector<FaceDetection> detected;
    detected.reserve(keep.size());

    for (auto i : keep) {
        auto box = candidateBoxes[i];
        std::cout << "Box[ centerX: " << box.centerX << ", centerY: " << box.centerY << ", width: " << box.width
                  << ", height:" << box.height << " ], Conf: " << box.confidence << std::endl;

        cv::Point topLeft(box.centerX - box.width / 2, box.centerY - box.height / 2);
        cv::Point bottomRight(box.centerX + box.width / 2, box.centerY + box.height / 2);

        // actual image size is bigger than 640;
        topLeft.x = static_cast<int>(topLeft.x * factorX);
        topLeft.y = static_cast<int>(topLeft.y * factorY);
        bottomRight.x = static_cast<int>(bottomRight.x * factorX);
        bottomRight.y = static_cast<int>(bottomRight.y * factorY);

        FaceDetection face;
        face.box = cv::Rect{topLeft.x, topLeft.y, bottomRight.x - topLeft.x, bottomRight.y - topLeft.y};
        face.confidence = box.confidence;
        detected.emplace_back(face);
    }

    result.faces = detected;

    return RET_OK;
}

} // namespace donde_toolkits::feature_extract::coreml_worker
