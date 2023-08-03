#include "donde/feature_extract/processor_factory.h"

#include "concurrent_processor.h"
#include "openvino_worker/worker_openvino.h"

#include <iostream>
#include <memory>
#include <type_traits>

using namespace std;

using json = nlohmann::json;

using donde_toolkits::feature_extract::ConcurrentProcessor;
using donde_toolkits::feature_extract::openvino_worker::AlignerWorker;
using donde_toolkits::feature_extract::openvino_worker::DetectorWorker;
using donde_toolkits::feature_extract::openvino_worker::FeatureWorker;
using donde_toolkits::feature_extract::openvino_worker::LandmarksWorker;

namespace donde_toolkits ::feature_extract {

Processor* ProcessorFactory::createDetector(const json& conf) { return {}; };
Processor* ProcessorFactory::createLandmarks(const json& conf) { return {}; };
Processor* ProcessorFactory::createAligner(const json& conf) { return {}; };
Processor* ProcessorFactory::createFeature(const json& conf) { return {}; };

} // namespace donde_toolkits::feature_extract
