#pragma once

#include <cmath>
#include <iostream>
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
    int version;
    std::string model;
    std::vector<float> raw;

    Feature() = default;
    Feature(std::vector<float>&& data) : raw(data){};

    float compare(const Feature& other) {
        float dot_product;
        float len1, len2;

        for (int i = 0; i < raw.size(); i++) {
            dot_product += raw[i] * other.raw[i];

            len1 += raw[i] * raw[i];
            len2 += other.raw[i] * other.raw[i];
        }

        len1 = sqrtf(len1);
        len2 = sqrtf(len2);

        float angle_cos = dot_product / (len1 * len2);
        if (angle_cos < -1.0)
            angle_cos = -1.0;
        if (angle_cos > 1.0)
            angle_cos = 1.0;

        float score = acosf(angle_cos);
        std::cout << "score" << score << std::endl;
        return score;
    };
};

struct FeatureResult {
    std::vector<Feature> face_features;
};

enum ValueType {
    ValueFrame,
    ValueDetectResult,
    ValueLandmarksResult,
    ValueAlignerResult,
    ValueFeatureResult,
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
