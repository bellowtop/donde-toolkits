#include "concurrent_processor.h"
#include "face_pipeline.h"
#include "types.h"
#include "utils.h"

#include <Poco/Logger.h>
#include <doctest/doctest.h>
#include <doctest/trompeloeil.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>
#include <string>

using namespace std;

using nlohmann::json;

TEST_CASE("FacePipeline can decode image binary to frame, aka cv::Mat.") {

    Logger& logger = Logger::get("test-pipeline");
    logger.setLevel("trace");

    json conf = R"(
 {
        "detector": {
            "model": "./contrib/models/face-detection-adas-0001.xml"
        }
 }
)"_json;

    string device_id = "CPU";

    FacePipeline pipeline{conf, device_id, logger};

    int concurrent = 1;
    auto detectorProcessor = std::make_shared<ConcurrentProcessor<DetectorWorker>>(
        conf, concurrent, device_id, logger);
    pipeline.Init(detectorProcessor, detectorProcessor, detectorProcessor, detectorProcessor);

    // read image data;
    // std::string img_path = "./contrib/data/test_image.png";
    // std::string img_path = "./contrib/data/zly_1.jpeg";
    std::string img_path = "./contrib/data/zly_2.jpeg";

    double len = std::filesystem::file_size(img_path);
    std::vector<uint8_t> image_data(len);

    std::ifstream f{img_path, std::ios::binary};
    f.read((char*)image_data.data(), image_data.size());

    Frame* frame = pipeline.Decode(image_data);
    DetectResult* result = pipeline.Detect(*frame);

    // scope exit
    std::shared_ptr<int> ggg(NULL, [&](int*) {
        delete frame;
        delete result;
    });

    std::cout << "result->box.x: " << result->box.x << std::endl;
    std::cout << "result->box.y: " << result->box.y << std::endl;
    std::cout << "result->box.width: " << result->box.width << std::endl;
    std::cout << "result->box.height: " << result->box.height << std::endl;

    CHECK(result->box.empty() == false);
    CHECK_GT(result->confidence, 0.8);
    CHECK_GT(result->box.size().width, 10);
    CHECK_GT(result->box.size().height, 10);

    drawRectangleInImage(img_path, result->box);

    // std::shared_ptr<Processor> a;

    // RetCode ret = pipe.Init(a, a, a, a);

    // CHECK(ret == RET_OK);

    // pipe.Terminate();

    CHECK("aa" == "aa");
};
