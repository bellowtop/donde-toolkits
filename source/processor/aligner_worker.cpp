#include "Poco/AutoPtr.h"
#include "Poco/Logger.h"
#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"
#include "concurrent_processor.h"
#include "pipeline_worker.h"
#include "opencv2/opencv.hpp"
#include "openvino/openvino.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include "types.h"
#include "utils.h"

#include <cassert>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <opencv2/core/cvdef.h>
#include <opencv2/core/types.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <string>
#include <vector>

using Poco::Notification;
using Poco::NotificationQueue;

using namespace Poco;

AlignerWorker::AlignerWorker(std::shared_ptr<NotificationQueue> ch) : Worker(ch) {}

AlignerWorker::~AlignerWorker() {
    // _channel.reset();
}

RetCode AlignerWorker::Init(json conf, int i, std::string device_id) {
    _name = "aligner-worker-" + std::to_string(i);
    init_log(_name);

    _id = i;
    _device_id = device_id;
    _conf = conf;

    return RET_OK;
}

void AlignerWorker::run() {
    for (;;) {
        Notification::Ptr pNf(_channel->waitDequeueNotification());

        if (pNf) {
            WorkMessage::Ptr msg = pNf.cast<WorkMessage>();
            if (msg) {
                if (msg->isQuitMessage()) {
                    break;
                }
                Value input = msg->getRequest();
                if (input.valueType != ValueLandmarksResult) {
                    _logger->error("AlignerWorker input value is not a ValueLandmarksResult! wrong "
                                   "valueType: {}",
                                   format(input.valueType));
                    continue;
                }
                std::shared_ptr<LandmarksResult> landmarks_result
                    = std::static_pointer_cast<LandmarksResult>(input.valuePtr);
                std::shared_ptr<AlignerResult> result = std::make_shared<AlignerResult>();

                RetCode ret = process(*landmarks_result, *result);
                _logger->debug("process ret: {}", ret);

                Value output{ValueAlignerResult, result};
                msg->setResponse(output);
            }
        } else {
            break;
        }
    }
}

RetCode AlignerWorker::process(const LandmarksResult& landmarks_result, AlignerResult& result) {
    if (landmarks_result.face_landmarks.size() != landmarks_result.faces.size()) {
        _logger->error("faces count is not equal to face landmarks count!");
        return RetCode::RET_ERR;
    }

    for (size_t i = 0; i < landmarks_result.faces.size(); i++) {
        cv::Mat aligned = align_face(landmarks_result.faces[i], landmarks_result.face_landmarks[i]);
        result.aligned_faces.push_back(aligned);

        // cv::imwrite("/tmp/face_" + std::to_string(i) + ".jpg", landmarks_result.faces[i]);
        // cv::imwrite("/tmp/face_aligned_" + std::to_string(i) + ".jpg", aligned);
    }

    return RetCode::RET_OK;
}

cv::Mat AlignerWorker::align_face(const cv::Mat& face_image,
                                  const std::vector<cv::Point2f>& landmarks) {
    // left eye: landmarks[0],  landmarks[1],
    // right eye: landmarks[2],  landmarks[3],
    // SEE
    // https://github.com/openvinotoolkit/open_model_zoo/blob/master/models/intel/facial-landmarks-35-adas-0002/README.md

    if (landmarks[1].x <= 0 || landmarks[2].x <= 0) {
        _logger->error("eye point x is small than 0");
        return cv::Mat();
    }

    cv::Point2f left_eye_center
        = {(landmarks[0].x + landmarks[1].x) * 0.5f, (landmarks[0].y + landmarks[1].y) * 0.5f};
    cv::Point2f right_eye_center
        = {(landmarks[2].x + landmarks[3].x) * 0.5f, (landmarks[2].y + landmarks[3].y) * 0.5f};

    std::cout << "left eye center x: " << left_eye_center.x << ", y : " << left_eye_center.y << std::endl;
    std::cout << "right eye center x: " << right_eye_center.x << ", y : " << right_eye_center.y << std::endl;

    cv::Point2f center = {(left_eye_center.x + right_eye_center.x) * 0.5f,
                          (left_eye_center.y + right_eye_center.y) * 0.5f};

    std::cout << "center x: " << center.x << ", y : " << center.y << std::endl;

    double dx = (right_eye_center.x - left_eye_center.x);
    double dy = (right_eye_center.y - left_eye_center.y);
    double angle = atan2(dy, dx) * 180.0 / CV_PI;
    std::cout << "dy: " << dy << ", dx: " << dx << ", angle: " << angle << std::endl;

    cv::Mat rot_mat = cv::getRotationMatrix2D(center, angle, 1.0f);
    cv::Mat warped;

    cv::warpAffine(face_image, warped, rot_mat, warped.size());

    return warped;
}
