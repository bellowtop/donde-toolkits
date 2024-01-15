#pragma once
#include "definitions.h"
#include "donde/uuid.h"
#include "opencv2/opencv.hpp"

// #include <google/protobuf/map.h>
#include <iostream>
#include <opencv2/core/types.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>

namespace donde_toolkits {

// inline std::map<std::string, std::string>
// convertMetadataToMap(const google::protobuf::Map<std::string, std::string>& metadata) {
//     // construct from iterator
//     std::map<std::string, std::string> m{metadata.begin(), metadata.end()};
//     return m;
// };

inline std::vector<float> convertFeatureBlobToFloats(const std::string& blob) {
    const char* char_ptr = reinterpret_cast<const char*>(blob.data());
    std::vector<float> float_vec(blob.size() / sizeof(float));
    for (size_t i = 0; i < float_vec.size(); i++) {
        float_vec[i] = *reinterpret_cast<const float*>(char_ptr + i * sizeof(float));
    }
    std::cout << "convertFeatureBlobToFloats: vector size: " << float_vec.size() << std::endl;

    return float_vec;
};

inline void drawRectangleInImage(const std::string& image_path, const std::vector<cv::Rect> rects) {
    cv::Mat img = cv::imread(image_path);

    for (auto& rect : rects) {
        // draw the bounding box
        cv::Point p1(rect.x, rect.y);
        cv::Point p2(rect.x + rect.width, rect.y + rect.height);
        cv::rectangle(img, p1, p2, cv::Scalar(0, 0, 255), 2, cv::LINE_8);
    }

    cv::imshow("Output", img);
    cv::waitKey(0);
}

inline std::string generate_uuid() { return uuid::generate_uuid_v4(); }

inline std::string replace_underscore_for_uuid(const std::string& u) {
    std::string ret(u);
    std::replace(ret.begin(), ret.end(), '-', '_');
    return ret;
}

} // namespace donde_toolkits
