#pragma once

#include "processor.h"

namespace donde_toolkits ::feature_extract {

class ProcessorFactory {

  public:
    static Processor* createDetector();
    static Processor* createLandmarks();
    static Processor* createAligner();
    static Processor* createFeature();
};

} // namespace donde_toolkits::feature_extract
