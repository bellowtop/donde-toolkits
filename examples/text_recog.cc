
#include "donde/definitions.h"
#include "donde/feature_extract/face_pipeline.h"
#include "donde/feature_extract/processor_factory.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <opencv2/imgcodecs.hpp>
#include <thread>

using donde_toolkits::Frame;
using donde_toolkits::OcrResult;
using donde_toolkits::feature_extract::FacePipeline;

using nlohmann::json;

int main(int argc, char** argv) {

    json conf = R"(
{
  "ocr": {
    "concurrent": 1,
    "device_id": "CPU",
    "lang": "chi_sim"
  }
}
                 )"_json;

    FacePipeline pipeline{conf};
    auto ocr = donde_toolkits::feature_extract::ProcessorFactory::createOcr();
    pipeline.InitOcrProcessor(ocr);
    std::shared_ptr<OcrResult> result;

    std::string image_path = "/Users/jiechen/Downloads/tess/1725093220415.jpg";

    // {
    //     std::ifstream ifs{image_path, std::ios::binary};
    //     if (!ifs) {
    //         std::cerr << "failed to open file";
    //         return 0;
    //     }
    //     ifs.seekg(0, std::ios::end);
    //     std::streamsize size = ifs.tellg();
    //     ifs.seekg(0, std::ios::beg);

    //     std::vector<unsigned char> img_data(size);
    //     if (!ifs.read(reinterpret_cast<char*>(img_data.data()), size)) {
    //         std::cerr << "failed to read from file";
    //         return 0;
    //     }
    //     std::shared_ptr<Frame> frame = pipeline.Decode(img_data);
    //     result = pipeline.TextRecognition(frame->image);
    // }

    {
        cv::Mat mat = cv::imread(image_path);
        result = pipeline.TextRecognition(mat);
    }

    std::cout << "ocr result: " << result->text << std::endl;
    pipeline.Terminate();

    return 0;
}
