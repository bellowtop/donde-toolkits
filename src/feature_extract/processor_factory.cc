#include "donde/feature_extract/processor_factory.h"

#include "common_worker/common_worker.h"
#include "concurrent_processor.h"
#include "openvino_worker/openvino_worker.h"

#include <iostream>
#include <memory>
#include <type_traits>

using json = nlohmann::json;

using donde_toolkits::feature_extract::ConcurrentProcessor;
using donde_toolkits::feature_extract::common_worker::OcrWorker;
using donde_toolkits::feature_extract::openvino_worker::AlignerWorker;
using donde_toolkits::feature_extract::openvino_worker::FeatureWorker;
using donde_toolkits::feature_extract::openvino_worker::LandmarksWorker;

// clang-format off
#if defined(__APPLE__)
#include "coreml_worker/coreml_worker.h"
using donde_toolkits::feature_extract::coreml_worker::DetectorWorker;
#else
#include "pytorch_worker/pytorch_worker.h"
using donde_toolkits::feature_extract::pytorch_worker::DetectorWorker;
#endif
// clang-format on

namespace donde_toolkits ::feature_extract {

Processor* ProcessorFactory::createOcr() { return new ConcurrentProcessor<OcrWorker>(); };
Processor* ProcessorFactory::createDetector() { return new ConcurrentProcessor<DetectorWorker>(); };
Processor* ProcessorFactory::createLandmarks() { return new ConcurrentProcessor<LandmarksWorker>(); };
Processor* ProcessorFactory::createAligner() { return new ConcurrentProcessor<AlignerWorker>(); };
Processor* ProcessorFactory::createFeature() { return new ConcurrentProcessor<FeatureWorker>(); };

} // namespace donde_toolkits::feature_extract
