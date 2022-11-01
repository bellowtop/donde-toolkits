#include "face_pipeline.h"

#include "Poco/Thread.h"
#include "concurrent_processor.h"

#include "openvino_worker/workers.h"

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"
#include "types.h"

#include <cstdint>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace openvino_worker;

using json = nlohmann::json;

/**
   json conf:
   {
       "detector": {
           "device_id": "CPU",
           "concurrent": 4,
           "model": "xxxx.xml"
       },
       "landmarks": {
           "device_id": "CPU",
           "concurrent": 4,
           "model": "xxxx.xml"
       },
       "aligner": {
           "device_id": "CPU",
           "concurrent": 4,
           "model": "xxxx.xml"
       },
       "feature": {
           "device_id": "CPU",
           "concurrent": 4,
           "model": "xxxx.xml"
       }
   }
 */

FacePipeline::FacePipeline(const json& conf)
    : _config(conf),
      _detectorProcessor(std::make_shared<ConcurrentProcessor<DetectorWorker>>()),
      _landmarksProcessor(std::make_shared<ConcurrentProcessor<LandmarksWorker>>()),
      _alignerProcessor(std::make_shared<ConcurrentProcessor<AlignerWorker>>()),
      _featureProcessor(std::make_shared<ConcurrentProcessor<FeatureWorker>>()) {}

RetCode FacePipeline::Init() {
    if (_config.contains("detector")) {
        RetCode ret = _detectorProcessor->Init(_config["detector"]);
    } else {
        spdlog::warn("no detector found in _config json, skip init detector processor");
    }

    if (_config.contains("landmarks")) {
        RetCode ret = _landmarksProcessor->Init(_config["landmarks"]);
    } else {
        spdlog::warn("no landmarks found in _config json, skip init landmarks processor");
    }

    if (_config.contains("aligner")) {
        RetCode ret = _alignerProcessor->Init(_config["aligner"]);
    } else {
        spdlog::warn("no aligner found in _config json, skip init aligner processor");
    }

    if (_config.contains("feature")) {
        RetCode ret = _featureProcessor->Init(_config["feature"]);
    } else {
        spdlog::warn("no feature found in _config json, skip init feature processor");
    }

    return RET_OK;
}

RetCode FacePipeline::Terminate() {
    if (_detectorProcessor->IsInited()) {
        RetCode ret = _detectorProcessor->Terminate();
    }

    if (_landmarksProcessor->IsInited()) {
        RetCode ret = _landmarksProcessor->Terminate();
    }

    if (_alignerProcessor->IsInited()) {
        RetCode ret = _alignerProcessor->Terminate();
    }

    if (_featureProcessor->IsInited()) {
        RetCode ret = _featureProcessor->Terminate();
    }

    return RET_OK;
}

std::shared_ptr<Frame> FacePipeline::Decode(const vector<uint8_t>& image_data) {
    cv::Mat image(cv::imdecode(image_data, cv::IMREAD_UNCHANGED));

    return std::make_shared<Frame>(image);
}

std::shared_ptr<DetectResult> FacePipeline::Detect(std::shared_ptr<Frame> frame) {
    Value input{ValueFrame, frame};
    // output.valuePtr memory is allocated by inner Process();
    Value output;

    RetCode ret = _detectorProcessor->Process(input, output);
    spdlog::info("FacePipeline::Detect ret: {}", ret);

    if (output.valueType != ValueDetectResult) {
        spdlog::error("Detect output is not ValueDetectResult, return empty result");
        return nullptr;
    }

    return std::static_pointer_cast<DetectResult>(output.valuePtr);
}

std::shared_ptr<LandmarksResult>
FacePipeline::Landmarks(std::shared_ptr<DetectResult> detect_result) {
    Value input{ValueDetectResult, detect_result};
    // output.valuePtr memory is allocated by inner Process();
    Value output;

    RetCode ret = _landmarksProcessor->Process(input, output);
    spdlog::info("FacePipeline::Landmarks ret: {}", ret);

    if (output.valueType != ValueLandmarksResult) {
        spdlog::error("Landmarks output is not ValueLandmarksResult, return empty result");
        return nullptr;
    }

    return std::static_pointer_cast<LandmarksResult>(output.valuePtr);
}

std::shared_ptr<AlignerResult>
FacePipeline::Align(std::shared_ptr<LandmarksResult> landmarks_result) {
    Value input{ValueLandmarksResult, landmarks_result};
    // output.valuePtr memory is allocated by inner Process();
    Value output;

    RetCode ret = _alignerProcessor->Process(input, output);
    spdlog::info("FacePipeline::Align ret: {}", ret);

    if (output.valueType != ValueAlignerResult) {
        spdlog::error("Align output is not ValueAlignerResult, return empty result");
        return nullptr;
    }

    return std::static_pointer_cast<AlignerResult>(output.valuePtr);
}

std::shared_ptr<FeatureResult>
FacePipeline::Extract(std::shared_ptr<AlignerResult> aligner_result) {
    Value input{ValueAlignerResult, aligner_result};
    // output.valuePtr memory is allocated by inner Process();
    Value output;

    RetCode ret = _featureProcessor->Process(input, output);
    spdlog::info("FacePipeline::Extract ret: {}", ret);

    if (output.valueType != ValueFeatureResult) {
        spdlog::error("Align output is not ValueFeatureResult, return empty result");
        return nullptr;
    }

    return std::static_pointer_cast<FeatureResult>(output.valuePtr);
}
