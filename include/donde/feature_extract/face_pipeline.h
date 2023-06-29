#pragma once

#include "donde/definitions.h"
#include "donde/feature_extract/processor.h"
// #include "faiss/Index2Layer.h"
#include "nlohmann/json.hpp"

#include <map>

using namespace Poco;
using namespace std;

using json = nlohmann::json;

namespace donde_toolkits {

namespace feature_extract {

class FacePipeline {
  public:
    virtual ~FacePipeline() = default;

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

} // namespace feature_extract

} // namespace donde_toolkits
