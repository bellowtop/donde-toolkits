#pragma once

#include "donde/feature_extract/processor.h"
#include "donde/feature_extract/worker.h"
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

    MOCK_METHOD(void, Die, ());
    ~MockProcessor() override { Die(); };
};

class MockWorker : public Worker {

  public:
    MOCK_METHOD(RetCode, Init, (json conf, int id, std::string device_id), (override));

    MOCK_METHOD(std::string, GetName, (), (override));

    MOCK_METHOD(void, run, (), (override));

    MOCK_METHOD(void, Die, ());
    ~MockWorker() override { Die(); };
};
