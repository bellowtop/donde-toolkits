#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <ostream>
#include <string>
#include <vector>

using namespace std;

enum RetCode { RET_OK, RET_ERR };

struct Frame {
    Frame(){};
    Frame(const cv::Mat& img) : image(img.clone()){};
    cv::Mat image;
};

struct FaceDetection {
    float confidence;
    cv::Rect box;
};

struct DetectResult {
    std::shared_ptr<Frame> frame;
    std::vector<FaceDetection> faces;
};

struct LandmarksResult {
    std::vector<cv::Mat> faces;
    std::vector<std::vector<cv::Point2f>> face_landmarks;
};

struct AlignerResult {
    std::vector<cv::Mat> aligned_faces;
};

struct Feature {
    std::vector<float> data;
};

enum ValueType {
    ValueFrame,
    ValueDetectResult,
    ValueLandmarksResult,
    ValueAlignerResult,
    ValueFeature
};

inline std::string format(const ValueType typ) {
    static std::map<ValueType, std::string> strings;
    if (strings.size() == 0) {
#define insert_elem(p) strings[p] = #p
        insert_elem(ValueFrame);
        insert_elem(ValueDetectResult);
        insert_elem(ValueLandmarksResult);
        insert_elem(ValueAlignerResult);
        insert_elem(ValueFeature);
#undef insert_elem
    }
    return strings[typ];
};

inline std::ostream& operator<<(std::ostream& out, const ValueType typ) {
    return out << format(typ);
};

struct Value {
    ValueType valueType;
    std::shared_ptr<void> valuePtr;
};
