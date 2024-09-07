#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>
#include <spdlog/common.h>
#include <string>
#include <cstdlib>

#include "Poco/NotificationQueue.h"

#include "common_worker.h"
#include "donde/definitions.h"
#include "donde/message.h"
#include "donde/utils.h"

using Poco::Notification;
using Poco::NotificationQueue;

namespace donde_toolkits ::feature_extract ::common_worker {

OcrWorker::OcrWorker(std::shared_ptr<MsgChannel> ch) : WorkerBaseImpl{ch}, _tesserctOcr(new tesseract::TessBaseAPI()) {}

OcrWorker::~OcrWorker() {
    if (_tesserctOcr) {
        _tesserctOcr->End();
    }
}

RetCode OcrWorker::Init(json conf, int i, std::string device_id) {
    _name = "ocr-worker-" + std::to_string(i);
    init_log(_name);

    _id = i;
    _device_id = device_id;
    _conf = conf;

    std::string lang = conf["lang"];

    std::string tess_data_env = "TESSDATA_PREFIX=.";
    // warning: cross-platform? windows use _putenv()
    if (putenv(const_cast<char*>(tess_data_env.c_str())) != 0) {
        _logger->error("failed to set env: TESSDATA_PREFIX=.");
    };

    int ret = _tesserctOcr->Init(NULL, lang.c_str());
    if (ret != 0) {
        _logger->error("OcrWorker conf with invalid `lang`: {}, Init() return: {}", lang, ret);
        return RET_ERR;
    }

    return RET_OK;
}

void OcrWorker::run() {
    for (;;) {
        // output is a blocking call.
        Notification::Ptr pNf = _channel->waitDequeueNotification();
        if (pNf.isNull()) {
            break;
        }
        WorkMessage<Value>::Ptr msg = pNf.cast<WorkMessage<Value>>();
        Value input = msg->getRequest();
        if (input.valueType != ValueFrame) {
            _logger->error("OcrWorker input value is not a frame! wrong valueType: {}", int(input.valueType));
            continue;
        }
        std::shared_ptr<Frame> f = std::static_pointer_cast<Frame>(input.valuePtr);

        std::shared_ptr<OcrResult> result = std::make_shared<OcrResult>();

        RetCode ret = process(f->image, *result);
        _logger->debug("process ret: {}", int(ret));

        Value output{ValueOcrResult, result};
        msg->setResponse(output);
    }
}

RetCode OcrWorker::process(const cv::Mat& image, OcrResult& result) {
    auto mat = image.clone();
    cv::cvtColor(mat, mat, cv::COLOR_BGR2RGBA);
    _tesserctOcr->SetImage(mat.data, mat.cols, mat.rows, 4, 4 * mat.cols);

    char* text = _tesserctOcr->GetUTF8Text();
    result.text = std::string(text);
    _logger->debug("text recognition is: {}", result.text);

    delete[] text;

    return RET_OK;
}

} // namespace donde_toolkits::feature_extract::common_worker
