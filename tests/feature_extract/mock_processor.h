#pragma once

#include "donde/feature_extract/processor.h"
#include "source/feature_extract/processor/concurrent_processor_impl.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace donde;
using namespace donde::feature_extract;

class MockProcessor : public Processor {

  public:
    MOCK_METHOD(RetCode, Init, (const json& cfg), (override));

    MOCK_METHOD(bool, IsInited, (), (override));

    MOCK_METHOD(RetCode, Process, (const Value& input, Value& output), (override));

    MOCK_METHOD(RetCode, Terminate, (), (override));

    MOCK_METHOD(std::string, GetName, (), (override));
};
