#include "detector_worker.h"

#include "Poco/AutoPtr.h"
#include "Poco/Logger.h"
#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"
#include "concurrent_processor.h"
#include "opencv2/opencv.hpp"
#include "openvino/openvino.hpp"
#include "types.h"
#include "utils.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

using Poco::Logger;
using Poco::Notification;
using Poco::NotificationQueue;

using namespace Poco;

DetectorWorker::DetectorWorker(std::shared_ptr<NotificationQueue> ch, Poco::Logger& parent)
    : Worker(ch), _logger(Logger::get(parent.name() + ".DetectorWorker")) {}

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
    const float* tensor_data = output.data<float>();

    int index = 0;

    // each batch in output tensor is a 7 floats array.
    // SEE
    // https://docs.openvino.ai/2019_R1/_face_detection_adas_0001_description_face_detection_adas_0001.html
    size_t object_size = 7;

    float image_id = tensor_data[0];
    float label = tensor_data[1];
    float conf = tensor_data[2];
    float x_min = tensor_data[3];
    float y_min = tensor_data[4];
    float x_max = tensor_data[5];
    float y_max = tensor_data[6];

    // poco format float with %hf
    // SEE https://kerpanic.wordpress.com/2016/10/31/errfmt-using-poco-logger/
    _logger.debug("image_id: %hf, label: %hf, conf: %hf, x_min: %hf, y_min: %hf, x_max: %hf, y_max: %hf",
                  image_id, label, conf, x_min, y_min, x_max, y_max);
}

RetCode DetectorWorker::Init(json conf, int i, std::string device_id) {
    _id = i;
    _name = "detector-worker-" + std::to_string(i);
    _device_id = device_id;
    _conf = conf;

    std::string model_path = conf["model"];

    _logger.information("loading model: %s", model_path);
    _logger.information("abs path: %s", std::filesystem::canonical(model_path).string());

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

    _compiled_model
        = std::make_shared<ov::CompiledModel>(std::move(core.compile_model(model, "CPU")));
    _infer_request
        = std::make_shared<ov::InferRequest>(std::move(_compiled_model->create_infer_request()));

    // warmup img
    std::string warmup_image = "/Users/jiechen/Documents/Images/white_background.jpg";
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
                    _logger.error("DetectorWorker input value is not a frame! wrong valueType: %d",
                                  input.valueType);
                    continue;
                }
                Frame* f = (Frame*)input.valuePtr;
                DetectResult result;
                RetCode ret = process(f->image, result);
                _logger.debug("process ret: ", ret);

                Value output{ValueDetectResult, (void*)&result};
                msg->setResponse(output);
            }
        } else {
            break;
        }
    }
}

// resize input img, and do inference
RetCode DetectorWorker::process(cv::Mat& img, DetectResult& result) {
    _logger.debug("resize image from [%d x %d] to [%d x %d] \n", img.cols, img.rows,
                  (int)_image_width, (int)_image_height);

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

    return RetCode::RET_OK;
}
