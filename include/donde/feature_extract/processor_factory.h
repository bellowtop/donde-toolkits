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
    static Processor* createDetector(const json& conf);
    static Processor* createLandmarks(const json& conf);
    static Processor* createAligner(const json& conf);
    static Processor* createFeature(const json& conf);
};

} // namespace donde_toolkits::feature_extract
