#include "detector_worker.h"

#include "Poco/AutoPtr.h"
#include "Poco/Logger.h"
#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"
#include "concurrent_processor.h"
#include "opencv2/opencv.hpp"
#include "openvino/openvino.hpp"

#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

#include "types.h"
#include "utils.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <memory>
#include <opencv2/core/types.hpp>
#include <string>

using Poco::Logger;
using Poco::Notification;
using Poco::NotificationQueue;

using namespace Poco;

DetectorWorker::DetectorWorker(std::shared_ptr<NotificationQueue> ch)
    : Worker(ch) {}

DetectorWorker::~DetectorWorker() {
    // _channel.reset();
}

/**

   conf:
   {
     "model": "../models/face-detection-adas-0001.xml"
   }

 */

void DetectorWorker::debugOutputTensor(const ov::Tensor& output) {
    ov::Shape shape = output.get_shape();
    const size_t batch_size = shape[0];
    const size_t face_numbers = shape[2];
    const float* tensor_data = output.data<float>();

    // each batch in output tensor is a 7 floats array.
    // SEE
    // https://docs.openvino.ai/2019_R1/_face_detection_adas_0001_description_face_detection_adas_0001.html
    size_t object_size = 7;

    for (size_t i = 0 ; i < face_numbers; i ++) {
        int offset = i * object_size;

        float image_id = tensor_data[offset+0];
        float label = tensor_data[offset+1];
        float conf = tensor_data[offset+2];
        float x_min = tensor_data[offset+3];
        float y_min = tensor_data[offset+4];
        float x_max = tensor_data[offset+5];
        float y_max = tensor_data[offset+6];

        if (conf < _min_confidence) {
            continue;
        }

        _logger->info("face-{}", i);
        _logger->info("\t image_id: {}, label: {}, conf: {}, x_min: {}, y_min: {}, x_max: {}, y_max: {} \n",
                       image_id, label, conf, x_min, y_min, x_max, y_max);
    }
}

RetCode DetectorWorker::Init(json conf, int i, std::string device_id) {
    _name = "detector-worker-" + std::to_string(i);
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

    // output shape: [1, 1, N, 7], where N is the number of detected bounding boxes
    // SEE https://docs.openvino.ai/2019_R1/_face_detection_adas_0001_description_face_detection_adas_0001.html
    ov::Shape output_shape = model->output().get_shape();
    _max_faces = output_shape[2];

    _compiled_model
        = std::make_shared<ov::CompiledModel>(std::move(core.compile_model(model, "CPU")));
    _infer_request
        = std::make_shared<ov::InferRequest>(std::move(_compiled_model->create_infer_request()));

    // warmup img
    std::string warmup_image = "./contrib/data/test_image_5_person.jpeg";
    cv::Mat img = cv::imread(warmup_image);

    DetectResult result;
    process(img, result);

    return RET_OK;
}

void DetectorWorker::run() {
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
                    _logger->error("DetectorWorker input value is not a frame! wrong valueType: {}",
                                  input.valueType);
                    continue;
                }
                std::shared_ptr<Frame> f = std::static_pointer_cast<Frame>(input.valuePtr);

                std::shared_ptr<DetectResult> result = std::make_shared<DetectResult>();
                // acquire! hold a reference to the frame.
                result->frame = f;

                RetCode ret = process(f->image, *result);
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
RetCode DetectorWorker::process(const cv::Mat& img, DetectResult& result) {
    _logger->debug("resize image from [{} x {}] to [{} x {}] \n", img.cols, img.rows,
                  (int)_image_width, (int)_image_height);

    int orig_img_width = img.size().width;
    int orig_img_height = img.size().height;

    const size_t data_length = img.channels() * _image_width * _image_height;
    std::shared_ptr<unsigned char> data_ptr;
    data_ptr.reset(new unsigned char[data_length], std::default_delete<unsigned char[]>());

    // size of each batch.
    const size_t image_size = ov::shape_size(_compiled_model->input().get_shape()) / _batch_size;
    assert(image_size == data_length);

    cv::Size new_size(_image_width, _image_height);
    cv::Mat resized_img(new_size, img.type(), data_ptr.get());
    cv::resize(img, resized_img, new_size);

    ov::Tensor input_tensor = _infer_request->get_input_tensor();

    std::memcpy(input_tensor.data<std::uint8_t>(), data_ptr.get(), image_size);

    _infer_request->infer();

    const ov::Tensor output_tensor = _infer_request->get_output_tensor();
    debugOutputTensor(output_tensor);

    const float* tensor_data = output_tensor.data<float>();
    std::vector<DetectFace> detected;
    detected.reserve(10);

    // each batch in output tensor is a 7 floats array.
    // SEE
    // https://docs.openvino.ai/2019_R1/_face_detection_adas_0001_description_face_detection_adas_0001.html
    size_t object_size = 7;

    for (size_t i = 0 ; i < _max_faces; i ++) {
        int offset = i * object_size;

        float image_id = tensor_data[offset+0];
        float label = tensor_data[offset+1];
        float conf = tensor_data[offset+2];
        float x_min = tensor_data[offset+3];
        float y_min = tensor_data[offset+4];
        float x_max = tensor_data[offset+5];
        float y_max = tensor_data[offset+6];

        if (conf < _min_confidence) {
            continue;
        }

        DetectFace face;

        face.box = cv::Rect{(int)(x_min * orig_img_width), (int)(y_min * orig_img_height),
                            (int)((x_max - x_min) * orig_img_width), (int)((y_max - y_min) * orig_img_height)};
        face.confidence = conf;
        detected.emplace_back(face);
    }

    result.faces = detected;

    return RetCode::RET_OK;
}
