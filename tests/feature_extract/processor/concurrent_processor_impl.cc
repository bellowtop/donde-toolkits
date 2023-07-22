#include "feature_extract/processor/concurrent_processor_impl.h"

#include "donde/definitions.h"
#include "donde/feature_extract/worker.h"
#include "donde/message.h"
#include "feature_extract/processor/worker_base_impl.h"
#include "tests/feature_extract/mock_processor.h"

#include <Poco/NotificationQueue.h>
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <thread>

using namespace std;
using namespace Poco;

using Poco::Notification;

using nlohmann::json;

using donde_toolkits::Feature;
using donde_toolkits::Frame;
using donde_toolkits::Value;
using donde_toolkits::ValueFeature;
using donde_toolkits::ValueFrame;
using donde_toolkits::feature_extract::ConcurrentProcessorImpl;
using donde_toolkits::feature_extract::WorkerBaseImpl;

TEST(FeatureExtract, ConcurrentProcessorHasConcurrentWorkers) {
    json conf = R"(
{
  "dummy": {
    "model": "./contrib/models/face-detection-adas-0001.xml",
    "concurrent": 5,
    "device_id": "CPU",
    "warmup": false
  }
})"_json;

    // start 5 workers, and all of them are running.
    static int count = 0;
    static int run_count = 0;

    class DummyWorker : public WorkerBaseImpl {
      public:
        DummyWorker(std::shared_ptr<MsgChannel> ch) : WorkerBaseImpl(ch) { count++; };
        void run() override { run_count++; };
    };

    ConcurrentProcessorImpl<DummyWorker> processor;
    processor.Init(conf["dummy"]);

    // wait all workers up, as workers are add to processor's execution-pool
    std::this_thread::sleep_for(std::chrono::seconds(1));

    EXPECT_EQ(count, 5);
    EXPECT_EQ(run_count, 5);

    EXPECT_EQ("aa", "aa");
};

TEST(FeatureExtract, ConcurrentProcessorComunicateWithWorkerUsingChannel) {
    static int processedMsg = 0;

    //
    // define a dummy worker for this test.
    //
    class DummyWorker : public WorkerBaseImpl {
      public:
        DummyWorker(std::shared_ptr<MsgChannel> ch) : WorkerBaseImpl(ch){};
        RetCode Init(json conf, int id, std::string device_id) override {
            EXPECT_NE(conf, nullptr);
            EXPECT_NE(id, -1);
            EXPECT_EQ(device_id, "CPU");
            return RET_OK;
        };

        void run() override {
            for (;;) {
                // output is a blocking call.
                Notification::Ptr pNf = _channel->waitDequeueNotification();
                std::cout << "pNf is null?" << pNf.isNull() << std::endl;

                if (pNf.isNull()) {
                    break;
                }

                WorkMessage<Value>::Ptr msg = pNf.cast<WorkMessage<Value>>();

                processedMsg++;

                Value input = msg->getRequest();
                std::cout << "get input, valueType: " << input.valueType << std::endl;

                EXPECT_EQ(input.valueType, ValueFrame);
                EXPECT_NE(input.valuePtr, nullptr);

                // allocate memory in heap, the caller is responsible to free it!
                std::shared_ptr<Feature> result = std::make_shared<Feature>();

                result->raw.resize(100);
                Value output{ValueFeature, result};

                msg->setResponse(output);
            }
        };
    };

    json conf = R"(
{
  "dummy": {
    "model": "./contrib/models/face-detection-adas-0001.xml",
    "concurrent": 2,
    "device_id": "CPU",
    "warmup": false
  }
})"_json;

    ConcurrentProcessorImpl<DummyWorker> processor;
    processor.Init(conf["dummy"]);

    std::shared_ptr<Frame> f = std::make_shared<Frame>();
    Value input{ValueFrame, f};

    Value output;
    processor.Process(input, output);

    EXPECT_EQ(output.valueType, ValueFeature);
    EXPECT_NE(output.valuePtr, nullptr);

    std::shared_ptr<Feature> feature = std::static_pointer_cast<Feature>(output.valuePtr);
    EXPECT_EQ(feature->raw.size(), 100);

    processor.Process(input, output);
    processor.Process(input, output);

    EXPECT_EQ(processedMsg, 3);

    processor.Terminate();

    EXPECT_EQ("aa", "aa");
};
