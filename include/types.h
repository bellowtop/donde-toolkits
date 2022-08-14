#pragma once

#include <cstdint>
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

struct AlignFace {
    cv::Mat face;
};

struct DetectResult {
    float confidence;
    cv::Rect box;
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
    void* valuePtr;
};
