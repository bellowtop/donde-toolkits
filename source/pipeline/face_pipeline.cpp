#include "face_pipeline.h"

#include "Poco/Logger.h"
#include "Poco/Thread.h"
#include "concurrent_processor.h"
#include "nlohmann/json.hpp"
#include "types.h"

#include <cstdint>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>

using namespace std;

using Poco::Logger;

using json = nlohmann::json;

/**
   json conf:
   {
       "detector": {
           "model": "xxxx.bin"
       },
       "aligner": {
           "model": "xxxx.bin"
       },
       "landmarks": {
           "model": "xxxx.bin"
       },
       "feature": {
           "model": "xxxx.bin"
       }
   }
 */

FacePipeline::FacePipeline(const json& conf, const std::string& device_id, const Logger& parent)
    : _config(conf), _device_id(device_id), _logger(Logger::get(parent.name() + ".FacePipeline")) {}

RetCode FacePipeline::Init(std::shared_ptr<Processor> detectorProcessor,
                           std::shared_ptr<Processor> alignerProcessor,
                           std::shared_ptr<Processor> landmarksProcessor,
                           std::shared_ptr<Processor> featureProcessor) {
    _detectorProcessor = detectorProcessor;
    _landmarksProcessor = landmarksProcessor;
    _alignerProcessor = alignerProcessor;
    _featureProcessor = featureProcessor;

    // Value *v = new Value;
    // v->valueType = ValueFrame;
    // v->valuePtr = &Frame{};
    // conf["id"] = reinterpret_cast<std::uintptr_t>(v);

    RetCode ret = _detectorProcessor->Init(_config["detector"]);

    // delete v;

    return RET_OK;
}

RetCode FacePipeline::Terminate() {
    RetCode ret = _detectorProcessor->Terminate();
    _logger.information("processor terminate ret: %d\n", ret);

    return RET_OK;
}

Frame* FacePipeline::Decode(const vector<uint8_t>& image_data) {
    cv::Mat image(cv::imdecode(image_data, cv::IMREAD_UNCHANGED));

    // we copy the image to the frame, image can free by itself.
    // and RVO happens here for frame return.
    Frame* frame = new Frame{image};
    return frame;
}

DetectResult* FacePipeline::Detect(const Frame& frame) {
    Value input{ValueFrame, (void*)(&frame)};
    // output.ptr memory is allocated by inner Process();
    Value output;

    RetCode ret = _detectorProcessor->Process(input, output);
    _logger.information("FacePipeline::Detect ret: %d\n", ret);

    if (output.valueType != ValueDetectResult) {
        _logger.error("Detect output is not ValueDetectResult, return empty result");
        return nullptr;
    }

    // this object is moved out, the caller is responsible for free memory.
    return (DetectResult *)output.valuePtr;
}
