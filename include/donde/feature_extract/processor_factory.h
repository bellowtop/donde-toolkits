#pragma once

#include "processor.h"

#include <iostream>
#include <memory>
#include <type_traits>

using namespace std;

using json = nlohmann::json;

namespace donde_toolkits ::feature_extract {

class ProcessorFactory {

  public:
    ~ProcessorFactory() = default;
    static Processor* createDetector();
    static Processor* createLandmarks();
    static Processor* createAligner();
    static Processor* createFeature();
};

} // namespace donde_toolkits::feature_extract
