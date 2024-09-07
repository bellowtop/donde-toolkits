#pragma once

#include "../worker.h"
#include "tesseract/baseapi.h"
#include "leptonica/allheaders.h"

namespace donde_toolkits ::feature_extract ::common_worker {

class OcrWorker : public WorkerBaseImpl {
  public:
    OcrWorker(std::shared_ptr<MsgChannel> ch);
    ~OcrWorker();

    RetCode Init(json conf, int id, std::string device_id) override;

    void run() override;

  private:
    RetCode process(const cv::Mat& frame, OcrResult& result);

    std::shared_ptr<tesseract::TessBaseAPI> _tesserctOcr;
};

} // namespace donde_toolkits::feature_extract::common_worker
