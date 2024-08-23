#include "Poco/AutoPtr.h"
#include "Poco/NotificationQueue.h"
#include "donde/definitions.h"
#include "donde/message.h"
#include "pytorch_utils.h"
#include "pytorch_worker.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <memory>
#include <opencv2/core/types.hpp>
#include <spdlog/common.h>
#include <string>

using Poco::Notification;
using Poco::NotificationQueue;

using namespace Poco;

namespace donde_toolkits ::feature_extract ::pytorch_worker {

DetectorWorker::DetectorWorker(std::shared_ptr<MsgChannel> ch)
    : WorkerBaseImpl{ch}, device{torch::cuda::is_available() ? torch::kCUDA : torch::kCPU} {}

DetectorWorker::~DetectorWorker() {
    // _channel.reset();
}

/**

   conf:
   {
       "model": "../models/face-detection-adas-0001.xml",
       "warmup": false
   }

 */

// void DetectorWorker::debugOutputTensor(const ov::Tensor& output) {
//     ov::Shape shape = output.get_shape();
//     const size_t batch_size = shape[0];
//     const size_t face_numbers = shape[2];
//     const float* tensor_data = output.data<float>();

//     // SEE
//     // https://docs.openvino.ai/2019_R1/_face_detection_adas_0001_description_face_detection_adas_0001.html

//     for (size_t i = 0; i < face_numbers; i++) {
//         int offset = i * _shape_dim;

//         float image_id = tensor_data[offset + 0];
//         float label = tensor_data[offset + 1];
//         float conf = tensor_data[offset + 2];
//         float x_min = tensor_data[offset + 3];
//         float y_min = tensor_data[offset + 4];
//         float x_max = tensor_data[offset + 5];
//         float y_max = tensor_data[offset + 6];

//         if (conf < _min_confidence) {
//             continue;
//         }

//         _logger->info("face-{}", i);
//         _logger->info("\t image_id: {}, label: {}, conf: {}, x_min: {}, y_min: {}, x_max: {}, y_max: {} \n",
//                       image_id,
//                       label,
//                       conf,
//                       x_min,
//                       y_min,
//                       x_max,
//                       y_max);
//     }
// }

RetCode DetectorWorker::Init(json conf, int i, std::string device_id) {
    _name = "detector-worker-" + std::to_string(i);
    init_log(_name);

    _id = i;
    _device_id = device_id;
    _conf = conf;

    std::string model_path = conf["model"];

    _logger->info("loading model: {}", model_path);
    _logger->info("absolute path: {}", std::filesystem::canonical(model_path).string());

    model_abs_path = std::filesystem::canonical(model_path).string();

    // Load the model (e.g. yolov8s.torchscript)
    yolo_model = torch::jit::load(model_abs_path);
    yolo_model.eval();
    yolo_model.to(device, torch::kFloat32);

    if (conf.contains("warmup") && conf["warmup"]) {
        // warmup img
        std::string warmup_image = "./contrib/data/test_image_5_person.jpeg";
        cv::Mat img = cv::imread(warmup_image);

        DetectResult result;
        process(img, result);
    }

    return RET_OK;
}

void DetectorWorker::run() {
    for (;;) {
        // output is a blocking call.
        Notification::Ptr pNf = _channel->waitDequeueNotification();
        if (pNf.isNull()) {
            break;
        }
        WorkMessage<Value>::Ptr msg = pNf.cast<WorkMessage<Value>>();
        Value input = msg->getRequest();
        if (input.valueType != ValueFrame) {
            _logger->error("DetectorWorker input value is not a frame! wrong valueType: {}", int(input.valueType));
            continue;
        }
        std::shared_ptr<Frame> f = std::static_pointer_cast<Frame>(input.valuePtr);

        std::shared_ptr<DetectResult> result = std::make_shared<DetectResult>();
        // acquire! hold a reference to the frame.
        result->frame = f;

        RetCode ret = process(f->image, *result);
        _logger->debug("process ret: {}", int(ret));

        Value output{ValueDetectResult, result};
        msg->setResponse(output);
    }
}

// resize input img, and do inference
RetCode DetectorWorker::process(const cv::Mat& image, DetectResult& result) {
    cv::Mat input_image;
    letterbox((cv::Mat&)image, input_image, {640, 640});

    torch::Tensor image_tensor
        = torch::from_blob(input_image.data, {input_image.rows, input_image.cols, 3}, torch::kByte).to(device);
    image_tensor = image_tensor.toType(torch::kFloat32).div(255);
    image_tensor = image_tensor.permute({2, 0, 1});
    image_tensor = image_tensor.unsqueeze(0);
    std::vector<torch::jit::IValue> inputs{image_tensor};

    // Inference
    torch::Tensor output = yolo_model.forward(inputs).toTensor().cpu();

    // NMS
    auto keep = non_max_suppression(output)[0];
    auto boxes = keep.index({Slice(), Slice(None, 4)});
    keep.index_put_({Slice(), Slice(None, 4)},
                    scale_boxes({input_image.rows, input_image.cols}, boxes, {image.rows, image.cols}));

    std::vector<FaceDetection> detected;
    detected.reserve(10);

    // Show the results
    for (int i = 0; i < keep.size(0); i++) {
        int cls = keep[i][5].item().toInt();
        // cls 0 is person
        if (cls != 0) {
            continue;
        }

        int x1 = keep[i][0].item().toFloat();
        int y1 = keep[i][1].item().toFloat();
        int x2 = keep[i][2].item().toFloat();
        int y2 = keep[i][3].item().toFloat();
        float conf = keep[i][4].item().toFloat();

        FaceDetection face;
        face.box = cv::Rect{x1, y1, x2 - x1, y2 - y1};
        face.confidence = conf;

        std::cout << "Box: [" << face.box << "]  Conf: " << face.confidence << std::endl;

        detected.emplace_back(face);
    }

    return RET_OK;
}

} // namespace donde_toolkits::feature_extract::pytorch_worker
