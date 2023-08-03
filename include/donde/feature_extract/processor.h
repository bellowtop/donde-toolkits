#pragma once

#include "donde/definitions.h"
#include "donde/message.h"

#include <iostream>
#include <memory>
#include <type_traits>

using namespace std;

using json = nlohmann::json;

namespace donde_toolkits ::feature_extract {

class Processor {

  public:
    virtual RetCode Init(const json& cfg) = 0;
    virtual ~Processor() = default;

    virtual bool IsInited() = 0;

    virtual RetCode Process(const Value& input, Value& output) = 0;

    virtual RetCode Terminate() = 0;

    virtual std::string GetName() = 0;
};

} // namespace donde_toolkits::feature_extract
