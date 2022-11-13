#include "concurrent_processor.h"
#include "face_pipeline.h"
#include "openvino_worker/workers.h"
#include "types.h"
#include "utils.h"

#include <Poco/Logger.h>
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <opencv2/core/types.hpp>
#include <opencv2/imgproc.hpp>
#include <string>

using namespace std;
using namespace openvino_worker;

using nlohmann::json;

TEST(FeatureExtract, FacePipelineCanDecodeImageBinaryToFrame) {

    json conf = R"(
{
  "detector": {
    "concurrent": 2,
    "device_id": "CPU",
    "model": "./contrib/models/face-detection-adas-0001.xml",
    "warmup": false
  }
}


)"_json;

    FacePipeline pipeline{conf};
    pipeline.Init();

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

        EXPECT_EQ(box.empty(), false);
        EXPECT_GT(box.size().width, 10);
        EXPECT_GT(box.size().height, 10);
        EXPECT_GT(detected_face.confidence, 0.8);

        boxes.push_back(box);
    }

    // drawRectangleInImage(img_path, boxes);

    // std::shared_ptr<Processor> a;

    // RetCode ret = pipe.Init(a, a, a, a);

    // EXPECT_EQ(ret, RET_OK);

    pipeline.Terminate();

    EXPECT_EQ("aa", "aa");
};

TEST(FeatureExtract, FacePipelineCanDetectLandmarksFromDetectResult) {

    json conf = R"(
 {
  "landmarks": {
    "concurrent": 2,
    "device_id": "CPU",
    "model": "./contrib/models/facial-landmarks-35-adas-0002.xml",
    "warmup": false
  },
  "aligner": {
    "concurrent": 2,
    "device_id": "CPU",
    "warmup": false
  }
}
)"_json;

    FacePipeline pipeline{conf};
    pipeline.Init();

    std::string warmup_image = "./contrib/data/test_image_5_person.jpeg";
    cv::Mat img = cv::imread(warmup_image);
    cv::Rect face_rect{839, 114, 256, 331};
    FaceDetection face = FaceDetection{0.9, face_rect};

    std::shared_ptr<DetectResult> detect_result = std::make_shared<DetectResult>();
    detect_result->faces.push_back(face);
    detect_result->frame = std::make_shared<Frame>(img);

    std::shared_ptr<LandmarksResult> result = pipeline.Landmarks(detect_result);

    // we only set one face
    EXPECT_EQ(result->faces.size(), 1);
    EXPECT_EQ(result->face_landmarks.size(), 1);
    EXPECT_EQ(result->face_landmarks[0].size(), 35);

    EXPECT_GT(result->face_landmarks[0][0].x, 0.0f);
    EXPECT_GT(result->face_landmarks[0][0].y, 0.0f);

    std::shared_ptr<AlignerResult> align_result = pipeline.Align(result);

    // drawRectangleInImage(img_path, boxes);

    // std::shared_ptr<Processor> a;

    // RetCode ret = pipe.Init(a, a, a, a);

    // EXPECT_EQ(ret, RET_OK);

    pipeline.Terminate();

    EXPECT_EQ("aa", "aa");
};

TEST(FeatureExtract, FacePipelineExtractFaceFeatureFromImageFile) {

    json conf = R"(
 {
  "detector": {
    "concurrent": 2,
    "device_id": "CPU",
    "model": "./contrib/models/face-detection-adas-0001.xml",
    "warmup": false
  },
  "landmarks": {
    "concurrent": 2,
    "device_id": "CPU",
    "model": "./contrib/models/facial-landmarks-35-adas-0002.xml",
    "warmup": false
  },
  "aligner": {
    "concurrent": 2,
    "device_id": "CPU",
    "warmup": false
  },
  "feature": {
    "concurrent": 2,
    "device_id": "CPU",
    "model": "./contrib/models/Sphereface.xml",
    "warmup": false
  }
}
)"_json;

    FacePipeline pipeline{conf};
    pipeline.Init();

    std::string img_path = "./contrib/data/test_image_5_person.jpeg";
    // std::string img_path = "./contrib/data/zly_1.jpeg";
    // std::string img_path = "./contrib/data/zly_2.jpeg";

    double len = std::filesystem::file_size(img_path);
    std::vector<uint8_t> image_data(len);

    std::ifstream f{img_path, std::ios::binary};
    f.read((char*)image_data.data(), image_data.size());

    std::shared_ptr<Frame> frame = pipeline.Decode(image_data);

    std::shared_ptr<DetectResult> detect_result = pipeline.Detect(frame);

    {
        for (auto& detected_face : detect_result->faces) {
            auto box = detected_face.box;
            spdlog::debug("confidence: {}", detected_face.confidence);
            spdlog::debug("box.x: {}", box.x);
            spdlog::debug("box.y: {}", box.y);
            spdlog::debug("box.width: {}", box.width);
            spdlog::debug("box.height: {}", box.height);
            EXPECT_EQ(box.empty(), false);
            EXPECT_GT(box.size().width, 10);
            EXPECT_GT(box.size().height, 10);
            EXPECT_GT(detected_face.confidence, 0.8);
        }
    }

    std::shared_ptr<LandmarksResult> landmarks_result = pipeline.Landmarks(detect_result);

    {
        EXPECT_EQ(landmarks_result->faces.size(), 5);
        EXPECT_EQ(landmarks_result->face_landmarks.size(), 5);
        EXPECT_EQ(landmarks_result->face_landmarks[0].size(), 35);

        EXPECT_GT(landmarks_result->face_landmarks[0][0].x, 0.0f);
        EXPECT_GT(landmarks_result->face_landmarks[0][0].y, 0.0f);
    }

    std::shared_ptr<AlignerResult> aligner_result = pipeline.Align(landmarks_result);

    std::shared_ptr<FeatureResult> feature_result = pipeline.Extract(aligner_result);

    {
        for (Feature& ft : feature_result->face_features) {
            std::cout << "face_feature:\n\t";
            for (float& f : ft.raw)
                std::cout << f << " ";
            std::cout << std::endl;
        }
    }

    pipeline.Terminate();

    EXPECT_EQ("aa", "aa");
};
