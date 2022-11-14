#pragma once
#include "donde/definitions.h"
#include "opencv2/opencv.hpp"
#include "openvino/openvino.hpp"
#include "uuid/uuid.h"

#include <google/protobuf/map.h>
#include <iostream>
#include <opencv2/core/types.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>

namespace donde {

inline void printInputAndOutputsInfo(const ov::Model& network) {
    std::cout << "model name: " << network.get_friendly_name() << std::endl;

    const std::vector<ov::Output<const ov::Node>> inputs = network.inputs();
    for (const ov::Output<const ov::Node>& input : inputs) {
        std::cout << "    inputs" << std::endl;

        const std::string name = input.get_names().empty() ? "NONE" : input.get_any_name();
        std::cout << "        input name: " << name << std::endl;

        const ov::element::Type type = input.get_element_type();
        std::cout << "        input type: " << type << std::endl;

        const ov::Shape shape = input.get_shape();
        std::cout << "        input shape: " << shape << std::endl;
    }

    const std::vector<ov::Output<const ov::Node>> outputs = network.outputs();
    for (const ov::Output<const ov::Node>& output : outputs) {
        std::cout << "    outputs" << std::endl;

        const std::string name = output.get_names().empty() ? "NONE" : output.get_any_name();
        std::cout << "        output name: " << name << std::endl;

        const ov::element::Type type = output.get_element_type();
        std::cout << "        output type: " << type << std::endl;

        const ov::Shape shape = output.get_shape();
        std::cout << "        output shape: " << shape << std::endl;
    }
}

inline std::map<std::string, std::string>
convertMetadataToMap(const google::protobuf::Map<std::string, std::string>& metadata) {
    // construct from iterator
    std::map<std::string, std::string> m{metadata.begin(), metadata.end()};
    return m;
};

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

inline std::string generate_uuid() {
    std::string ret;
    ret.resize(36); // 16 bytes => 32 hex ascii chars represent, then 4 seperators.

    uuid_t uuid;
    uuid_generate(uuid);
    uuid_unparse_lower(uuid, ret.data());

    return ret;
}

inline std::string replace_underscore_for_uuid(const std::string& u) {
    std::string ret(u);
    std::replace(ret.begin(), ret.end(), '-', '_');
    return ret;
}

} // namespace donde
