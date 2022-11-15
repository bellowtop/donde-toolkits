#include "Poco/Thread.h"
#include "donde/definitions.h"
#include "donde/feature_extract/processor.h"
#include "nlohmann/json.hpp"
#include "source/feature_extract/pipeline/face_pipeline_imp.h"
#include "source/feature_extract/processor/concurrent_processor_impl.h"
#include "source/feature_extract/processor/openvino_worker/workers_impl.h"
#include "spdlog/spdlog.h"

#include <cstdint>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace donde::feature_extract::openvino_worker;

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
namespace donde {

namespace feature_extract {

FacePipelineImpl::FacePipelineImpl(const json& conf) : _config(conf) {}

RetCode FacePipelineImpl::Init(Processor* detector, Processor* landmarks, Processor* aligner,
                               Processor* feature) {

    _detectorProcessor.reset(detector);
    _landmarksProcessor.reset(landmarks);
    _alignerProcessor.reset(aligner);
    _featureProcessor.reset(feature);

    if (_config.contains("detector")) {
        if (_detectorProcessor->Init(_config["detector"]) != RetCode::RET_OK) {
            spdlog::warn("cannot init detector processor");
            return RetCode::RET_ERR;
        };
    } else {
        spdlog::warn("no detector found in _config json, skip init detector processor");
    }

    if (_config.contains("landmarks")) {
        if (_landmarksProcessor->Init(_config["landmarks"]) != RetCode::RET_OK) {
            spdlog::warn("cannot init landmarks processor");
            return RetCode::RET_ERR;
        };
    } else {
        spdlog::warn("no landmarks found in _config json, skip init landmarks processor");
    }

    if (_config.contains("aligner")) {
        if (_alignerProcessor->Init(_config["aligner"]) != RetCode::RET_OK) {
            spdlog::warn("cannot init aligner processor");
            return RetCode::RET_ERR;
        };
    } else {
        spdlog::warn("no aligner found in _config json, skip init aligner processor");
    }

    if (_config.contains("feature")) {
        if (_featureProcessor->Init(_config["feature"]) != RetCode::RET_OK) {
            spdlog::warn("cannot init feature processor");
            return RetCode::RET_ERR;
        };
    } else {
        spdlog::warn("no feature found in _config json, skip init feature processor");
    }

    return RET_OK;
}

RetCode FacePipelineImpl::Terminate() {
    if (_detectorProcessor && _detectorProcessor->IsInited()) {
        RetCode ret = _detectorProcessor->Terminate();
    }

    if (_landmarksProcessor && _landmarksProcessor->IsInited()) {
        RetCode ret = _landmarksProcessor->Terminate();
    }

    if (_alignerProcessor && _alignerProcessor->IsInited()) {
        RetCode ret = _alignerProcessor->Terminate();
    }

    if (_featureProcessor && _featureProcessor->IsInited()) {
        RetCode ret = _featureProcessor->Terminate();
    }

    return RET_OK;
}

std::shared_ptr<Frame> FacePipelineImpl::Decode(const vector<uint8_t>& image_data) {
    cv::Mat image(cv::imdecode(image_data, cv::IMREAD_UNCHANGED));

    return std::make_shared<Frame>(image);
}

std::shared_ptr<DetectResult> FacePipelineImpl::Detect(std::shared_ptr<Frame> frame) {
    Value input{ValueFrame, frame};
    // output.valuePtr memory is allocated by inner Process();
    Value output;

    RetCode ret = _detectorProcessor->Process(input, output);
    spdlog::info("FacePipelineImpl::Detect ret: {}", ret);

    if (output.valueType != ValueDetectResult) {
        spdlog::error("Detect output is not ValueDetectResult, return empty result");
        return nullptr;
    }

    return std::static_pointer_cast<DetectResult>(output.valuePtr);
}

std::shared_ptr<LandmarksResult>
FacePipelineImpl::Landmarks(std::shared_ptr<DetectResult> detect_result) {
    Value input{ValueDetectResult, detect_result};
    // output.valuePtr memory is allocated by inner Process();
    Value output;

    RetCode ret = _landmarksProcessor->Process(input, output);
    spdlog::info("FacePipelineImpl::Landmarks ret: {}", ret);

    if (output.valueType != ValueLandmarksResult) {
        spdlog::error("Landmarks output is not ValueLandmarksResult, return empty result");
        return nullptr;
    }

    return std::static_pointer_cast<LandmarksResult>(output.valuePtr);
}

std::shared_ptr<AlignerResult>
FacePipelineImpl::Align(std::shared_ptr<LandmarksResult> landmarks_result) {
    Value input{ValueLandmarksResult, landmarks_result};
    // output.valuePtr memory is allocated by inner Process();
    Value output;

    RetCode ret = _alignerProcessor->Process(input, output);
    spdlog::info("FacePipelineImpl::Align ret: {}", ret);

    if (output.valueType != ValueAlignerResult) {
        spdlog::error("Align output is not ValueAlignerResult, return empty result");
        return nullptr;
    }

    return std::static_pointer_cast<AlignerResult>(output.valuePtr);
}

std::shared_ptr<FeatureResult>
FacePipelineImpl::Extract(std::shared_ptr<AlignerResult> aligner_result) {
    Value input{ValueAlignerResult, aligner_result};
    // output.valuePtr memory is allocated by inner Process();
    Value output;

    RetCode ret = _featureProcessor->Process(input, output);
    spdlog::info("FacePipelineImpl::Extract ret: {}", ret);

    if (output.valueType != ValueFeatureResult) {
        spdlog::error("Align output is not ValueFeatureResult, return empty result");
        return nullptr;
    }

    return std::static_pointer_cast<FeatureResult>(output.valuePtr);
}

} // namespace feature_extract
} // namespace donde
