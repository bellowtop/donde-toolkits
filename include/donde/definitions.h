#pragma once

#include <stdint.h>
#ifndef SPDLOG_FMT_EXTERNAL

#    define SPDLOG_FMT_EXTERNAL
#endif

#include "spdlog/spdlog.h"
// #include "utils.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
//// #include <msgpack.hpp>
//// #include <msgpack/adaptor/define_decl.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/opencv.hpp>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

namespace donde_toolkits {

enum RetCode { RET_OK, RET_ERR };

struct Frame {
    Frame(){};
    Frame(const cv::Mat& img) : image(img){};
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
    std::vector<float> raw;
    std::string model;
    int version;
    int dimension;

    // 4 bytes in header,
    //   (16 bits for version, max value 131072),
    //   (10 bits for dimension, max value 2048, well, that's a huge dimension for feature)
    //   (6  bits for reserved.)
    // note body size can be calculated from dimension
    static std::array<uint8_t, 4> SerializeHeader(const Feature& ft);
    static std::vector<uint8_t> SerializeBody(const Feature& ft);

    Feature(std::vector<float>&& data) : raw{data}, dimension(data.size()){};
    Feature(std::vector<float>&& data, std::string&& model, int version)
        : raw{data}, model{model}, version{version}, dimension(data.size()){};

    Feature() = default;
    Feature(const Feature& lhs) = default;
    Feature(Feature&& lhs) = default;
    Feature& operator=(const Feature& lhs) = default;
    Feature& operator=(Feature&& lhs) = default;
    ~Feature() = default;

    void debugPrint() const {
        std::stringstream ss;
        ss << "ft:"
           << " dim:" << raw.size() << " model:" << model << " version:" << version << std::endl;
        for (float f : raw) {
            ss << f << " ";
        }
        ss << std::endl;
        std::cout << ss.str();
        spdlog::debug(ss.str());
    };

    const std::vector<float> normalize(const std::vector<float> raw_floats) const {
        std::vector norm(raw_floats.size(), 0.f);
        float len = 0.f;
        for (size_t i = 0; i < raw_floats.size(); i++) {
            len += raw_floats[i] * raw_floats[i];
        }
        len = sqrtf(len);
        for (size_t i = 0; i < norm.size(); i++) {
            norm[i] = raw_floats[i] / len;
        }
        return norm;
    };

    float compare(const Feature& other) const {
        float score = 0.f;

        auto norm1 = normalize(raw);
        auto norm2 = normalize(other.raw);

        for (size_t i = 0; i < norm1.size(); i++) {
            score += norm1[i] * norm2[i];
        }

        spdlog::debug("score: {}", score);

        return score;
    };

    // message pack
    //// MSGPACK_DEFINE(model, version, dimension, raw)
};

struct FeatureScore {
    Feature feature;
    float score;
};

struct FeatureScoreComparator {
    bool operator()(const FeatureScore& lhs, const FeatureScore& rhs) {
        return lhs.score > rhs.score;
    }
};

template <int size>
Feature gen_feature_dim() {
    std::vector<float> raw(size);
    for (int i = 0; i < size; i++) {
        // [0.000 ~ 1]
        raw[i] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    }
    return Feature(std::move(raw), "test-model-face", size);
}

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

inline std::string format_value_type(const ValueType typ) {
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
    return out << format_value_type(typ);
};

struct Value {
    ValueType valueType;
    std::shared_ptr<void> valuePtr;
};

} // namespace donde_toolkits
