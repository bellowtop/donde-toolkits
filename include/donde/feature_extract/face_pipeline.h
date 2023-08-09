#pragma once

#include "donde/definitions.h"
#include "nlohmann/json.hpp"
#include "processor.h"

#include <map>

using namespace Poco;
using namespace std;

using json = nlohmann::json;

namespace donde_toolkits ::feature_extract {

class IFacePipeline {
  public:
    virtual ~IFacePipeline() = default;

    virtual const json& GetConfig() = 0;

    // take owner of these input pointers.
    // implementations should release them in dtor.
    virtual RetCode Init(Processor* detector, Processor* landmarks, Processor* aligner,
                         Processor* feature)
        = 0;

    virtual RetCode Terminate() = 0;

    virtual std::shared_ptr<Frame> Decode(const vector<uint8_t>& image_data) = 0;

    virtual std::shared_ptr<DetectResult> Detect(const std::shared_ptr<Frame> frame) = 0;

    virtual std::shared_ptr<LandmarksResult>
    Landmarks(const std::shared_ptr<DetectResult> detect_result) = 0;

    virtual std::shared_ptr<AlignerResult>
    Align(const std::shared_ptr<LandmarksResult> landmarks_result) = 0;

    virtual std::shared_ptr<FeatureResult> Extract(std::shared_ptr<AlignerResult> aligner_result)
        = 0;
};

// forward declearation.
class FacePipelineImpl;
class FacePipeline : public IFacePipeline {
  public:
    FacePipeline(const json& config);
    // this dtor declaration is necessary. and its implementation
    // must be placed in face_pipeline.cc, because we use pimpl with std::unique_ptr
    ~FacePipeline();

    const json& GetConfig() override;

    RetCode Init(Processor* detector, Processor* landmarks, Processor* aligner,
                 Processor* feature) override;

    RetCode Terminate() override;

    std::shared_ptr<Frame> Decode(const vector<uint8_t>& image_data) override;

    std::shared_ptr<DetectResult> Detect(const std::shared_ptr<Frame> frame) override;

    std::shared_ptr<LandmarksResult>
    Landmarks(const std::shared_ptr<DetectResult> detect_result) override;

    std::shared_ptr<AlignerResult>
    Align(const std::shared_ptr<LandmarksResult> landmarks_result) override;

    std::shared_ptr<FeatureResult> Extract(std::shared_ptr<AlignerResult> aligner_result) override;

  public:
    std::unique_ptr<FacePipelineImpl> pimp;
};

} // namespace donde_toolkits::feature_extract
