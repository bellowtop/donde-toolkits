#include "donde/feature_extract/face_pipeline.h"

#include "concurrent_processor.h"
#include "donde/definitions.h"
#include "donde/feature_extract/processor.h"
#include "face_pipeline_impl.h"
#include "openvino_worker/worker_openvino.h"

#include <cstdint>
#include <memory>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>

using namespace std;

using namespace donde_toolkits::feature_extract::openvino_worker;

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

namespace donde_toolkits ::feature_extract {

FacePipeline::FacePipeline(const json& conf) : pimp(new FacePipelineImpl(conf)) {}
FacePipeline::~FacePipeline(){};

RetCode FacePipeline::Init(Processor* detector, Processor* landmarks, Processor* aligner,
                           Processor* feature) {
    return pimp->Init(detector, landmarks, aligner, feature);
}

RetCode FacePipeline::Terminate() { return pimp->Terminate(); }

const json& FacePipeline::GetConfig() { return pimp->GetConfig(); };

std::shared_ptr<Frame> FacePipeline::Decode(const vector<uint8_t>& image_data) {
    return pimp->Decode(image_data);
}

std::shared_ptr<DetectResult> FacePipeline::Detect(std::shared_ptr<Frame> frame) {
    return pimp->Detect(frame);
}

std::shared_ptr<LandmarksResult>
FacePipeline::Landmarks(std::shared_ptr<DetectResult> detect_result) {
    return pimp->Landmarks(detect_result);
}

std::shared_ptr<AlignerResult>
FacePipeline::Align(std::shared_ptr<LandmarksResult> landmarks_result) {
    return pimp->Align(landmarks_result);
}

std::shared_ptr<FeatureResult>
FacePipeline::Extract(std::shared_ptr<AlignerResult> aligner_result) {
    return pimp->Extract(aligner_result);
}

} // namespace donde_toolkits::feature_extract
