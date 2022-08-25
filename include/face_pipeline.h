#pragma once

#include "concurrent_processor.h"
#include "nlohmann/json.hpp"
#include "types.h"

#include <map>

using namespace Poco;
using namespace std;

using json = nlohmann::json;

class FacePipeline {
  public:
    FacePipeline(const json& config);

    const json& GetConfig() { return _config; };

    RetCode Init();

    RetCode Terminate();

    std::shared_ptr<Frame> Decode(const vector<uint8_t>& image_data);

    std::shared_ptr<DetectResult> Detect(const std::shared_ptr<Frame> frame);

    std::shared_ptr<LandmarksResult> Landmarks(const std::shared_ptr<DetectResult> detect_result);

    std::shared_ptr<AlignerResult> Align(const std::shared_ptr<LandmarksResult> landmarks_result);

    std::shared_ptr<FeatureResult> Extract(std::shared_ptr<AlignerResult> aligner_result);

  private:
    json _config;

    std::shared_ptr<Processor> _detectorProcessor;
    std::shared_ptr<Processor> _landmarksProcessor;
    std::shared_ptr<Processor> _alignerProcessor;
    std::shared_ptr<Processor> _featureProcessor;
};
