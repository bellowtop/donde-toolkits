#include "Poco/AutoPtr.h"
#include "Poco/Logger.h"
#include "Poco/Notification.h"
#include "Poco/NotificationQueue.h"
#include "donde/definitions.h"
#include "donde/feature_extract/worker_openvino_impl.h"
#include "donde/message.h"
#include "donde/utils.h"
#include "opencv2/opencv.hpp"
#include "openvino/openvino.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

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
namespace donde_toolkits ::feature_extract ::openvino_worker {

FeatureWorker::FeatureWorker(std::shared_ptr<MsgChannel> ch) : WorkerBaseImpl(ch) {}

FeatureWorker::~FeatureWorker() {
    // _channel.reset();
}

/**

   conf:
   {
     "model": "../models/Sphereface.xml"
     "warmup": false
   }

 */

void FeatureWorker::debugOutputTensor(const ov::Tensor& output_tensor) {
    const float* tensor_data = output_tensor.data<float>();

    // only one batch yet.
    size_t batch_idx = 0;
    const std::vector<float> arr(tensor_data, tensor_data + _feature_length);
    std::cout << "feature-extract: ";
    for (float f : arr) {
        std::cout << f << " ";
    }
    std::cout << std::endl;
}

RetCode FeatureWorker::Init(json conf, int i, std::string device_id) {
    _name = "feature-extract-worker-" + std::to_string(i);
    init_log(_name);

    _id = i;
    _device_id = device_id;
    _conf = conf;

    std::string model_path = conf["model"];

    _logger->info("loading model: {}", model_path);
    _logger->info("absolute path: {}", std::filesystem::canonical(model_path).string());

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

    // Face embeddings, name - fc5, shape - 1, 512, output data format - B, C, where
    // SEE
    // https://github.com/openvinotoolkit/open_model_zoo/blob/master/models/public/Sphereface/README.md
    ov::Shape output_shape = model->output().get_shape();
    _feature_length = output_shape[1];

    _compiled_model
        = std::make_shared<ov::CompiledModel>(std::move(core.compile_model(model, _device_id)));
    _infer_request
        = std::make_shared<ov::InferRequest>(std::move(_compiled_model->create_infer_request()));

    if (conf.contains("warmup") && conf["warmup"]) {
        // TODO
        // warmup img
        std::string warmup_image = "./contrib/data/test_image_5_person.jpeg";
        cv::Mat img = cv::imread(warmup_image);
        cv::Rect face{839, 114, 256, 331};
    }

    return RET_OK;
}

void FeatureWorker::run() {
    for (;;) {
        // output is a blocking call.
        Notification::Ptr pNf = _channel->waitDequeueNotification();
        if (pNf.isNull()) {
            break;
        }
        WorkMessage<Value>::Ptr msg = pNf.cast<WorkMessage<Value>>();
        Value input = msg->getRequest();
        if (input.valueType != ValueAlignerResult) {
            _logger->error("FeatureWorker input value is not a ValueAlignerResult! wrong "
                           "valueType: {}",
                           format_value_type(input.valueType));
            continue;
        }
        std::shared_ptr<AlignerResult> aligner_result
            = std::static_pointer_cast<AlignerResult>(input.valuePtr);
        std::shared_ptr<FeatureResult> result = std::make_shared<FeatureResult>();

        RetCode ret = process(*aligner_result, *result);
        _logger->debug("process ret: {}", int(ret));

        Value output{ValueFeatureResult, result};
        msg->setResponse(output);
    }
}

// resize input img, and do inference
RetCode FeatureWorker::process(const AlignerResult& aligner_result, FeatureResult& result) {
    result.face_features.reserve(aligner_result.aligned_faces.size());

    for (const cv::Mat& face_img : aligner_result.aligned_faces) {

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

        // debugOutputTensor(output_tensor);

        Feature ft;
        ft.raw.reserve(_feature_length);
        ft.version = 10000;
        ft.model = "Sphereface";

        // only one batch yet.
        size_t batch_idx = 0;
        size_t offset = batch_idx * _feature_length;
        for (size_t i = 0; i < _feature_length; i++) {
            float x = tensor_data[offset + i];
            ft.raw.push_back(x);
        }

        result.face_features.push_back(ft);
    }

    return RetCode::RET_OK;
}
} // namespace donde_toolkits::feature_extract::openvino_worker
