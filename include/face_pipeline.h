#pragma once

#include "concurrent_processor.h"
#include "detector_worker.h"
#include "nlohmann/json.hpp"
#include "types.h"

#include <map>

using namespace Poco;
using namespace std;

using json = nlohmann::json;

class FacePipeline {
  public:
    FacePipeline(const json& config, const std::string& device_id);

    const json& GetConfig() { return _config; };

    RetCode Init(std::shared_ptr<Processor> detectorProcessor,
                 std::shared_ptr<Processor> alignerProcessor,
                 std::shared_ptr<Processor> landmarksProcessor,
                 std::shared_ptr<Processor> featureProcessor);

    RetCode Terminate();

    std::shared_ptr<Frame> Decode(const vector<uint8_t>& image_data);

    std::shared_ptr<DetectResult> Detect(const std::shared_ptr<Frame> frame);

    std::shared_ptr<LandmarksResult> Landmarks(const std::shared_ptr<DetectResult> detect_result);

    std::shared_ptr<AlignerResult> Align(const std::shared_ptr<LandmarksResult> landmarks_result);


  private:
    json _config;
    std::string _device_id;

    std::shared_ptr<Processor> _detectorProcessor;
    std::shared_ptr<Processor> _landmarksProcessor;
    std::shared_ptr<Processor> _alignerProcessor;
    std::shared_ptr<Processor> _featureProcessor;
};
