#include "donde/definitions.h"
#include "donde/feature_extract/face_pipeline.h"
#include "donde/feature_extract/processor_factory.h"
#include "donde/utils.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

using namespace donde_toolkits::feature_extract;

using donde_toolkits::AlignerResult;
using donde_toolkits::DetectResult;
using donde_toolkits::FaceDetection;
using donde_toolkits::Feature;
using donde_toolkits::FeatureResult;
using donde_toolkits::Frame;
using donde_toolkits::LandmarksResult;

using nlohmann::json;

int main(int argc, char** argv) {
    json conf = R"(
    {
        "detector": {
          "concurrent": 1,
          "device_id": "CPU",
          "model": "./contrib/models/face-detection-adas-0001.xml",
          "warmup": false
        }
    }
)"_json;

    // init face pipeline, with detector only.
    FacePipeline pipeline{conf};
    auto detector = donde_toolkits::feature_extract::ProcessorFactory::createDetector();
    pipeline.Init(detector, nullptr, nullptr, nullptr);

    // read image data;
    // std::string img_path = "./contrib/data/test_image2.png";
    std::string img_path = "./contrib/data/test_image_5_person.jpeg";
    // std::string img_path = "./contrib/data/zly_1.jpeg";
    // std::string img_path = "./contrib/data/zly_2.jpeg";

    double len = std::filesystem::file_size(img_path);
    std::vector<uint8_t> image_data(len);

    std::ifstream f{img_path, std::ios::binary};
    f.read((char*)image_data.data(), image_data.size());

    // test decode
    std::shared_ptr<Frame> frame = pipeline.Decode(image_data);

    // test detect face
    std::shared_ptr<DetectResult> result = pipeline.Detect(frame);

    std::vector<cv::Rect> boxes(result->faces.size());

    for (auto& detected_face : result->faces) {
        auto box = detected_face.box;
        std::cout << "box.x: " << box.x << std::endl;
        std::cout << "box.y: " << box.y << std::endl;
        std::cout << "box.width: " << box.width << std::endl;
        std::cout << "box.height: " << box.height << std::endl;

        spdlog::debug("pipeline.Detect DetectResult.confidence: {}", detected_face.confidence);
        boxes.push_back(box);
    }

    return 0;
}
