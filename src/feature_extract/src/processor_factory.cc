#include "donde/feature_extract/processor_factory.h"

#include "concurrent_processor.h"
#include "openvino_worker/openvino_worker.h"

#include <iostream>
#include <memory>
#include <type_traits>



using json = nlohmann::json;

using donde_toolkits::feature_extract::ConcurrentProcessor;
using donde_toolkits::feature_extract::openvino_worker::AlignerWorker;
using donde_toolkits::feature_extract::openvino_worker::DetectorWorker;
using donde_toolkits::feature_extract::openvino_worker::FeatureWorker;
using donde_toolkits::feature_extract::openvino_worker::LandmarksWorker;

namespace donde_toolkits ::feature_extract {

Processor* ProcessorFactory::createDetector() { return new ConcurrentProcessor<DetectorWorker>(); };
Processor* ProcessorFactory::createLandmarks() {
    return new ConcurrentProcessor<LandmarksWorker>();
};
Processor* ProcessorFactory::createAligner() { return new ConcurrentProcessor<AlignerWorker>(); };
Processor* ProcessorFactory::createFeature() { return new ConcurrentProcessor<FeatureWorker>(); };

} // namespace donde_toolkits::feature_extract
