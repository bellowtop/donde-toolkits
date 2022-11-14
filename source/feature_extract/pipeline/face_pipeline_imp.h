#pragma once

#include "donde/definitions.h"
#include "donde/feature_extract/face_pipeline.h"
#include "donde/feature_extract/processor.h"
#include "faiss/Index2Layer.h"
#include "nlohmann/json.hpp"

#include <map>

using namespace Poco;
using namespace std;

using json = nlohmann::json;

namespace donde {

namespace feature_extract {

class FacePipelineImpl : public FacePipeline {
  public:
    FacePipelineImpl(const json& config);

    const json& GetConfig() override { return _config; };

    RetCode Init() override;

    RetCode Terminate() override;

    std::shared_ptr<Frame> Decode(const vector<uint8_t>& image_data) override;

    std::shared_ptr<DetectResult> Detect(const std::shared_ptr<Frame> frame) override;

    std::shared_ptr<LandmarksResult>
    Landmarks(const std::shared_ptr<DetectResult> detect_result) override;

    std::shared_ptr<AlignerResult>
    Align(const std::shared_ptr<LandmarksResult> landmarks_result) override;

    std::shared_ptr<FeatureResult> Extract(std::shared_ptr<AlignerResult> aligner_result) override;

  private:
    json _config;

    std::shared_ptr<Processor> _detectorProcessor;
    std::shared_ptr<Processor> _landmarksProcessor;
    std::shared_ptr<Processor> _alignerProcessor;
    std::shared_ptr<Processor> _featureProcessor;
};

} // namespace feature_extract

} // namespace donde
