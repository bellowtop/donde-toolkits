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

using Poco::Notification;
using Poco::NotificationQueue;

using namespace Poco;

LandmarksWorker::LandmarksWorker(std::shared_ptr<NotificationQueue> ch) : Worker(ch) {}

LandmarksWorker::~LandmarksWorker() {
    // _channel.reset();
}

/**

   conf:
   {
     "model": "../models/facial-landmarks-35-adas-0002.xml"
   }

 */

void LandmarksWorker::debugOutputTensor(const ov::Tensor& output) {
    ov::Shape shape = output.get_shape();
    const size_t batch_size = shape[0];
    const size_t face_numbers = shape[2];
    const float* tensor_data = output.data<float>();

    // each batch in output tensor is a 7 floats array.
    // SEE
    // https://docs.openvino.ai/2019_R1/_face_detection_adas_0001_description_face_detection_adas_0001.html
    size_t object_size = 7;

    for (size_t i = 0; i < face_numbers; i++) {
        int offset = i * object_size;

        float image_id = tensor_data[offset + 0];
        float label = tensor_data[offset + 1];
        float conf = tensor_data[offset + 2];
        float x_min = tensor_data[offset + 3];
        float y_min = tensor_data[offset + 4];
        float x_max = tensor_data[offset + 5];
        float y_max = tensor_data[offset + 6];

        if (conf < _min_confidence) {
            continue;
        }

        _logger->info("face-{}", i);
        _logger->info(
            "\t image_id: {}, label: {}, conf: {}, x_min: {}, y_min: {}, x_max: {}, y_max: {} \n",
            image_id, label, conf, x_min, y_min, x_max, y_max);
    }
}

RetCode LandmarksWorker::Init(json conf, int i, std::string device_id) {
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
    _landmarks_length = output_shape[2];

    _compiled_model
        = std::make_shared<ov::CompiledModel>(std::move(core.compile_model(model, "CPU")));
    _infer_request
        = std::make_shared<ov::InferRequest>(std::move(_compiled_model->create_infer_request()));

    // warmup img
    std::string warmup_image = "./contrib/data/test_image_5_person.jpeg";
    cv::Mat img = cv::imread(warmup_image);

    cv::Rect face{839, 114, 256, 331};
    DetectResult detect_result;
    detect_result.faces.push_back(DetectFace{0.9, face});

    LandmarksResult result;
    process(detect_result, result);

    return RET_OK;
}

void LandmarksWorker::run() {
    for (;;) {
        Notification::Ptr pNf(_channel->waitDequeueNotification());

        if (pNf) {
            WorkMessage::Ptr msg = pNf.cast<WorkMessage>();
            if (msg) {
                if (msg->isQuitMessage()) {
                    break;
                }
                Value input = msg->getRequest();
                if (input.valueType != ValueFrame) {
                    _logger->error(
                        "LandmarksWorker input value is not a frame! wrong valueType: {}",
                        input.valueType);
                    continue;
                }
                std::shared_ptr<DetectResult> detect_result = std::static_pointer_cast<DetectResult>(input.valuePtr);
                std::shared_ptr<LandmarksResult> result = std::make_shared<LandmarksResult>();

                RetCode ret = process(*detect_result, *result);
                _logger->debug("process ret: {}", ret);

                Value output{ValueDetectResult, result};
                msg->setResponse(output);
            }
        } else {
            break;
        }
    }
}

// resize input img, and do inference
RetCode LandmarksWorker::process(const DetectResult& detect_result, LandmarksResult& result) {

    return RetCode::RET_OK;
}
