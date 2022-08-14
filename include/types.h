#pragma once

#include <cstdint>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <vector>

using namespace std;

enum RetCode { RET_OK, RET_ERR };

struct Frame {
    Frame(){};
    Frame(const cv::Mat& img) : image(img.clone()){};
    cv::Mat image;
};

struct DetectFace {
    float confidence;
    cv::Rect box;
};

struct DetectResult {
    std::shared_ptr<Frame> frame;
    std::vector<DetectFace> faces;
};

struct LandmarksResult {
    std::vector<float> normalized;
};

struct Feature {
    std::vector<float> data;
};

enum ValueType { ValueFrame, ValueDetectResult, ValueLandmarksResult, ValueFeature };

struct Value {
    ValueType valueType;
    std::shared_ptr<void> valuePtr;
};
