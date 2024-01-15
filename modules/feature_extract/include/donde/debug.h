#pragma once
#include "donde/definitions.h"
#include "donde/uuid.h"
#include "opencv2/opencv.hpp"
#include "openvino/openvino.hpp"

// #include <google/protobuf/map.h>
#include <iostream>
#include <opencv2/core/types.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>

namespace donde_toolkits {

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

} // namespace donde_toolkits
