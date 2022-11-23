#include "face_service.h"

#include "Poco/Format.h"
#include "Poco/Timestamp.h"
#include "api/common.pb.h"
#include "api/feature_extract.grpc.pb.h"
#include "api/feature_extract.pb.h"
#include "config.h"
#include "donde/definitions.h"
#include "donde/utils.h"
#include "nlohmann/json.hpp"
#include "source/feature_extract/processor/concurrent_processor_impl.h"
#include "source/feature_extract/processor/openvino_worker/workers_impl.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <cassert>
#include <grpc/grpc.h>
#include <grpc/impl/codegen/status.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/status.h>
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

using com::sekirocc::feature_extract::DetectionRequest;
using com::sekirocc::feature_extract::DetectionResponse;

using com::sekirocc::feature_extract::ExtractionRequest;
using com::sekirocc::feature_extract::ExtractionResponse;

using com::sekirocc::feature_extract::CompareRequest;
using com::sekirocc::feature_extract::CompareResponse;

using com::sekirocc::feature_extract::FaceService;

using com::sekirocc::common::Rect;
using com::sekirocc::common::ResultCode;

using com::sekirocc::common::FaceFeature;
using com::sekirocc::common::FaceRectangle;

using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;

using json = nlohmann::json;

using donde::feature_extract::ConcurrentProcessorImpl;
using donde::feature_extract::openvino_worker::AlignerWorker;
using donde::feature_extract::openvino_worker::DetectorWorker;
using donde::feature_extract::openvino_worker::FeatureWorker;
using donde::feature_extract::openvino_worker::LandmarksWorker;

FaceServiceImpl::FaceServiceImpl(Config& server_config)
    : config(server_config),
      device_id(server_config.get_device_id()),
      pipeline(config.get_pipeline_config()){};

FaceServiceImpl::~FaceServiceImpl(){};

void FaceServiceImpl::Start() {
    // pipeline is responsible to release
    auto detector = new ConcurrentProcessorImpl<DetectorWorker>();
    auto landmarks = new ConcurrentProcessorImpl<LandmarksWorker>();
    auto aligner = new ConcurrentProcessorImpl<AlignerWorker>();
    auto feature = new ConcurrentProcessorImpl<FeatureWorker>();

    pipeline.Init(detector, landmarks, aligner, feature);
}

void FaceServiceImpl::Stop() { pipeline.Terminate(); };

Status FaceServiceImpl::Detect(ServerContext* context, const DetectionRequest* request,
                               DetectionResponse* response) {

    if (!request->has_image()) {
        return Status(StatusCode::INVALID_ARGUMENT, "invalid request image", "image is null");
    }
    const std::string& image_data = request->image().data();
    const std::vector<uint8_t> image_char_vec(image_data.begin(), image_data.end());

    // auto release.
    std::shared_ptr<donde::Frame> frame = pipeline.Decode(image_char_vec);
    std::shared_ptr<donde::DetectResult> result = pipeline.Detect(frame);

    response->set_code(ResultCode::OK);

    for (auto& detected_face : result->faces) {
        // fill response
        FaceRectangle* face_rect = response->add_face_rects();
        face_rect->set_quality(1.0f); // TODO quality unimplemented
        face_rect->set_confidence(detected_face.confidence);

        Rect* rect = face_rect->mutable_rectangle();
        rect->mutable_point()->set_x(detected_face.box.x);
        rect->mutable_point()->set_y(detected_face.box.y);
        rect->mutable_size()->set_width(detected_face.box.width);
        rect->mutable_size()->set_height(detected_face.box.height);

        spdlog::debug("pipeline.Detect DetectResult.confidence: {}", detected_face.confidence);
    }

    // int count = request->requests_size();
    // std::vector<Frame*> frames;
    // frames.reserve(count);

    // if (count == 0) {
    //     return Status(StatusCode::INVALID_ARGUMENT, "requests size is 0");
    // }

    // for (int i = 0; i < count; i++) {
    //     const FaceDetectRequest& req = request->requests(i);
    //     if (!req.has_image()) {
    //         return Status(StatusCode::INVALID_ARGUMENT, "invalid request image", "image is
    //         null");
    //     }
    //     const std::string& image_data = req.image().data();
    //     const std::vector<uint8_t> image_char_vec(image_data.begin(), image_data.end());
    //     Frame* frame = pipeline.Decode(image_char_vec);
    //     frames.push_back(frame);
    // }
    // DetectResult* ret = pipeline.Detect(*frames[0]);

    return Status::OK;
}

Status FaceServiceImpl::ExtractFeature(ServerContext* context, const ExtractionRequest* request,
                                       ExtractionResponse* response) {

    if (!request->has_image()) {
        return Status(StatusCode::INVALID_ARGUMENT, "invalid request image", "image is null");
    }
    const std::string& image_data = request->image().data();
    const std::vector<uint8_t> image_char_vec(image_data.begin(), image_data.end());

    // auto release.
    std::shared_ptr<donde::Frame> frame = pipeline.Decode(image_char_vec);
    std::shared_ptr<donde::DetectResult> detect_result = pipeline.Detect(frame);
    std::shared_ptr<donde::LandmarksResult> landmarks_result = pipeline.Landmarks(detect_result);
    std::shared_ptr<donde::AlignerResult> aligner_result = pipeline.Align(landmarks_result);
    std::shared_ptr<donde::FeatureResult> feature_result = pipeline.Extract(aligner_result);

    if (detect_result->faces.size() != feature_result->face_features.size()) {
        return Status(StatusCode::INTERNAL, "internal error",
                      "face length is not equal with feature lenth");
    }

    response->set_code(ResultCode::OK);
    for (auto& detected_face : detect_result->faces) {
        FaceRectangle* face_rect = response->add_face_rects();
        face_rect->set_quality(1.0f); // TODO quality unimplemented
        face_rect->set_confidence(detected_face.confidence);

        Rect* rect = face_rect->mutable_rectangle();
        rect->mutable_point()->set_x(detected_face.box.x);
        rect->mutable_point()->set_y(detected_face.box.y);
        rect->mutable_size()->set_width(detected_face.box.width);
        rect->mutable_size()->set_height(detected_face.box.height);
    }

    for (auto& ft : feature_result->face_features) {
        ft.debugPrint();

        // convert ft => face_feat.
        FaceFeature* face_feat = response->add_face_features();
        const char* p = reinterpret_cast<const char*>(&ft.raw[0]);
        size_t size = sizeof(float) * ft.raw.size();

        std::string str;
        str.resize(size);
        std::copy(p, p + size, str.data());

        face_feat->set_blob(str);
        face_feat->set_version(ft.version);
        face_feat->set_model(ft.model);
    }

    return Status::OK;
}

Status FaceServiceImpl::CompareFeature(ServerContext* context, const CompareRequest* request,
                                       CompareResponse* response) {

    FaceFeature one = request->one();
    FaceFeature two = request->two();

    const std::string& one_blob = one.blob();
    const std::string& two_blob = two.blob();

    donde::Feature ft1(donde::convertFeatureBlobToFloats(one_blob), std::string(one.model()),
                       one.version());
    donde::Feature ft2(donde::convertFeatureBlobToFloats(two_blob), std::string(two.model()),
                       two.version());

    if (ft1.raw.size() != ft2.raw.size()) {
        return Status(StatusCode::INVALID_ARGUMENT, "invalid request feature",
                      "number one & number two feature size no equal");
    }

    ft1.debugPrint();
    ft2.debugPrint();

    float score = ft1.compare(ft2);

    response->set_code(ResultCode::OK);
    response->set_score(score);

    return Status::OK;
}
