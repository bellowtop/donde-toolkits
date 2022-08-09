#include "concurrent_processor.h"
#include "face_pipeline.h"
#include "types.h"

#include <Poco/Logger.h>
#include <doctest/doctest.h>
#include <doctest/trompeloeil.hpp>

#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>

#include <memory>
#include <nlohmann/json.hpp>

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
    std::string img_path = "./contrib/data/test_image.png";

    double len = std::filesystem::file_size(img_path);
    std::vector<uint8_t> image_data(len);

    std::ifstream f{img_path, std::ios::binary};
    f.read((char *)image_data.data(), image_data.size());

    // const std::vector<uint8_t> image_char_vec(buf, buf + len);
    Frame* frame = pipeline.Decode(image_data);
    DetectResult* result = pipeline.Detect(*frame);

    std::cout << "result->box.x: " << result->box.x << std::endl;
    std::cout << "result->box.y: " << result->box.y << std::endl;
    std::cout << "result->box.width: " << result->box.width << std::endl;
    std::cout << "result->box.height: " << result->box.height << std::endl;

    CHECK(result->box.empty() == false);
    CHECK_GT(result->confidence, 0.8);
    CHECK_GT(result->box.size().width, 10);
    CHECK_GT(result->box.size().height, 10);

    delete result;

    // std::shared_ptr<Processor> a;

    // RetCode ret = pipe.Init(a, a, a, a);

    // CHECK(ret == RET_OK);

    // pipe.Terminate();

    CHECK("aa" == "aa");
};
