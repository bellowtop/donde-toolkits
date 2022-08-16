#include "Poco/AutoPtr.h"
#include "Poco/Logger.h"
#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"
#include "concurrent_processor.h"
#include "processor_worker.h"
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

LandmarksWorker::LandmarksWorker(std::shared_ptr<NotificationQueue> ch) : Worker(ch) {}

LandmarksWorker::~LandmarksWorker() {
    // _channel.reset();
}

/**

   conf:
   {
     "model": "../models/facial-landmarks-35-adas-0002.xml"
     "warmup": false
   }

 */

void LandmarksWorker::debugOutputTensor(const ov::Tensor& output_tensor) {
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

RetCode LandmarksWorker::Init(json conf, int i, std::string device_id) {
    _name = "landmarks-worker-" + std::to_string(i);
    init_log(_name);

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

        DetectResult detect_result;
        detect_result.faces.push_back(FaceDetection{0.9, face});
        detect_result.frame = std::make_shared<Frame>(img);

        LandmarksResult result;
        process(detect_result, result);
    }

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
                if (input.valueType != ValueDetectResult) {
                    _logger->error("LandmarksWorker input value is not a ValueDetectResult! wrong "
                                   "valueType: {}",
                                   format(input.valueType));
                    continue;
                }
                std::shared_ptr<DetectResult> detect_result
                    = std::static_pointer_cast<DetectResult>(input.valuePtr);
                std::shared_ptr<LandmarksResult> result = std::make_shared<LandmarksResult>();

                RetCode ret = process(*detect_result, *result);
                _logger->debug("process ret: {}", ret);

                Value output{ValueLandmarksResult, result};
                msg->setResponse(output);
            }
        } else {
            break;
        }
    }
}

// resize input img, and do inference
RetCode LandmarksWorker::process(const DetectResult& detect_result, LandmarksResult& result) {

    result.faces.reserve(detect_result.faces.size());
    result.face_landmarks.reserve(detect_result.faces.size());

    for (auto& detect_face : detect_result.faces) {
        // img is smallface, image(rect) => small image
        cv::Mat face_img = (detect_result.frame->image)(detect_face.box);
        result.faces.push_back(face_img);

        const size_t data_length = face_img.channels() * _image_width * _image_height;
        std::shared_ptr<unsigned char> data_ptr;
        data_ptr.reset(new unsigned char[data_length], std::default_delete<unsigned char[]>());

        // size of each batch.
        const size_t image_size
            = ov::shape_size(_compiled_model->input().get_shape()) / _batch_size;
        assert(image_size == data_length);

        cv::Mat resized_img(face_img);
        if (static_cast<int>(_image_width) != face_img.size().width
            || static_cast<int>(_image_height) != face_img.size().height) {
            cv::resize(face_img, resized_img, cv::Size(_image_width, _image_height));
        }

        ov::Tensor input_tensor = _infer_request->get_input_tensor();

        std::memcpy(input_tensor.data<std::uint8_t>(), data_ptr.get(), image_size);

        _infer_request->infer();

        const ov::Tensor output_tensor = _infer_request->get_output_tensor();
        const float* tensor_data = output_tensor.data<float>();

        debugOutputTensor(output_tensor);

        std::vector<cv::Point2f> lm(_landmarks_length / 2);

        // int face_img_width = face_img.size().width;
        // int face_img_height = face_img.size().height;

        // only one batch yet.
        size_t batch_idx = 0;
        size_t offset = batch_idx * _landmarks_length;
        for (size_t i = 0; i < _landmarks_length / 2; i++) {
            float x = tensor_data[offset + 2 * i];
            float y = tensor_data[offset + 2 * i + 1];
            lm[i] = cv::Point2f(x, y);

            // x, y is the real points in original big image. but we don't need them.
            // int x = static_cast<int>(tensor_data[offset + 2 * i] * face_img_width
            //                          + detect_face.box.tl().x);
            // int y = static_cast<int>(tensor_data[offset + 2 * i + 1] * face_img_height
            //                          + detect_face.box.tl().y);
        }

        result.face_landmarks.push_back(lm);
    }

    return RetCode::RET_OK;
}
