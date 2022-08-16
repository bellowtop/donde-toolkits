#include "Poco/AutoPtr.h"
#include "Poco/Logger.h"
#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"
#include "concurrent_processor.h"
#include "detector_worker.h"
#include "opencv2/opencv.hpp"
#include "openvino/openvino.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include "types.h"
#include "utils.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <memory>
#include <opencv2/core/types.hpp>
#include <string>
#include <vector>

using Poco::Notification;
using Poco::NotificationQueue;

using namespace Poco;

FeatureWorker::FeatureWorker(std::shared_ptr<NotificationQueue> ch) : Worker(ch) {}

FeatureWorker::~FeatureWorker() {
    // _channel.reset();
}

/**

   conf:
   {
     "model": "../models/facial-landmarks-35-adas-0002.xml"
     "warmup": false
   }

 */

void FeatureWorker::debugOutputTensor(const ov::Tensor& output_tensor) {
    const float* tensor_data = output_tensor.data<float>();

    // only one batch yet.
    size_t batch_idx = 0;
    const std::vector<float> arr(tensor_data, tensor_data + _landmarks_length);
    std::cout << "landmarks: ";
    for (float f : arr) {
        std::cout << f << " ";
    }
    std::cout << std::endl;
}

RetCode FeatureWorker::Init(json conf, int i, std::string device_id) {
    _name = "landmarks-worker-" + std::to_string(i);
    _logger = spdlog::stdout_color_mt(_name);

    _id = i;
    _device_id = device_id;
    _conf = conf;

    std::string model_path = conf["model"];

    _logger->info("loading model: {}", model_path);
    _logger->info("abs path: {}", std::filesystem::canonical(model_path).string());

    ov::Core core;
    std::shared_ptr<ov::Model> model = core.read_model(model_path);
    printInputAndOutputsInfo(*model);

    OPENVINO_ASSERT(model->inputs().size() == 1, "Sample supports models with 1 input only");
    OPENVINO_ASSERT(model->outputs().size() == 1, "Sample supports models with 1 output only");

    ov::preprocess::PrePostProcessor ppp(model);
    ov::preprocess::InputInfo& input_info = ppp.input();

    // input data is NHWC
    ov::Layout tensor_layout{"NHWC"};
    input_info.tensor().set_element_type(ov::element::u8).set_layout(tensor_layout);
    // but the model input need nchw, so the convertion begin.
    input_info.model().set_layout("NCHW");
    // model output is float32
    ppp.output().tensor().set_element_type(ov::element::f32);

    model = ppp.build();

    _batch_size = 1;
    ov::set_batch(model, _batch_size);

    ov::Shape input_shape = model->input().get_shape();
    _image_width = input_shape[ov::layout::width_idx(tensor_layout)];
    _image_height = input_shape[ov::layout::height_idx(tensor_layout)];

    // output shape: [1, 70], where 70 means [x0, y0, x1, y1....], 35 points
    // SEE
    // https://github.com/openvinotoolkit/open_model_zoo/blob/master/models/intel/facial-landmarks-35-adas-0002/README.md
    ov::Shape output_shape = model->output().get_shape();
    _landmarks_length = output_shape[1];

    _compiled_model
        = std::make_shared<ov::CompiledModel>(std::move(core.compile_model(model, "CPU")));
    _infer_request
        = std::make_shared<ov::InferRequest>(std::move(_compiled_model->create_infer_request()));

    bool need_warnmup = conf["warmup"];
    if (need_warnmup) {
        // warmup img
        std::string warmup_image = "./contrib/data/test_image_5_person.jpeg";
        cv::Mat img = cv::imread(warmup_image);
        cv::Rect face{839, 114, 256, 331};

    }

    return RET_OK;
}

void FeatureWorker::run() {
    for (;;) {
        Notification::Ptr pNf(_channel->waitDequeueNotification());

        if (pNf) {
            WorkMessage::Ptr msg = pNf.cast<WorkMessage>();
            if (msg) {
                if (msg->isQuitMessage()) {
                    break;
                }
                Value input = msg->getRequest();
                if (input.valueType != ValueAlignerResult) {
                    _logger->error("FeatureWorker input value is not a ValueAligner! wrong "
                                   "valueType: {}",
                                   format(input.valueType));
                    continue;
                }
                std::shared_ptr<AlignerResult> detect_result
                    = std::static_pointer_cast<AlignerResult>(input.valuePtr);
                std::shared_ptr<FeatureResult> result = std::make_shared<FeatureResult>();

                RetCode ret = process(*detect_result, *result);
                _logger->debug("process ret: {}", ret);

                Value output{ValueFeatureResult, result};
                msg->setResponse(output);
            }
        } else {
            break;
        }
    }
}

// resize input img, and do inference
RetCode FeatureWorker::process(const AlignerResult& detect_result, FeatureResult& result) {

    return RetCode::RET_OK;
}
