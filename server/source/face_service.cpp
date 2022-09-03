#include "face_service.h"

#include "Poco/Format.h"
#include "Poco/Timestamp.h"
#include "concurrent_processor.h"
#include "config.h"
#include "face_pipeline.h"
#include "gen/pb-cpp/server.grpc.pb.h"
#include "gen/pb-cpp/server.pb.h"
#include "nlohmann/json.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include "types.h"

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

using com::sekirocc::face_service::DetectionRequest;
using com::sekirocc::face_service::DetectionResponse;

using com::sekirocc::face_service::ExtractionRequest;
using com::sekirocc::face_service::ExtractionResponse;

using com::sekirocc::face_service::CompareRequest;
using com::sekirocc::face_service::CompareResponse;

using com::sekirocc::face_service::FaceService;

using com::sekirocc::face_service::Rect;
using com::sekirocc::face_service::ResultCode;

using com::sekirocc::face_service::FaceFeature;
using com::sekirocc::face_service::FaceRectangle;

using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;

using json = nlohmann::json;

FaceServiceImpl::FaceServiceImpl(Config& server_config)
    : config(server_config),
      device_id(server_config.get_device_id()),
      pipeline(config.get_pipeline_config()){};
FaceServiceImpl::~FaceServiceImpl(){};

void FaceServiceImpl::Start() { pipeline.Init(); };

void FaceServiceImpl::Stop() { pipeline.Terminate(); };

Status FaceServiceImpl::Detect(ServerContext* context, const DetectionRequest* request,
                               DetectionResponse* response) {

    if (!request->has_image()) {
        return Status(StatusCode::INVALID_ARGUMENT, "invalid request image", "image is null");
    }
    const std::string& image_data = request->image().data();
    const std::vector<uint8_t> image_char_vec(image_data.begin(), image_data.end());

    // auto release.
    std::shared_ptr<Frame> frame = pipeline.Decode(image_char_vec);
    std::shared_ptr<DetectResult> result = pipeline.Detect(frame);

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
    std::shared_ptr<Frame> frame = pipeline.Decode(image_char_vec);
    std::shared_ptr<DetectResult> detect_result = pipeline.Detect(frame);
    std::shared_ptr<LandmarksResult> landmarks_result = pipeline.Landmarks(detect_result);
    std::shared_ptr<AlignerResult> aligner_result = pipeline.Align(landmarks_result);
    std::shared_ptr<FeatureResult> feature_result = pipeline.Extract(aligner_result);

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

    const char* one_char_ptr = reinterpret_cast<const char*>(one_blob.data());
    std::vector<float> one_float_vec(one_blob.size() / sizeof(float));
    for (size_t i = 0; i < one_float_vec.size(); i++) {
        one_float_vec[i] = *reinterpret_cast<const float*>(one_char_ptr + i * sizeof(float));
    }
    std::cout << "vector one size: " << one_float_vec.size() << std::endl;

    Feature ft1(std::move(one_float_vec), std::string(one.model()), one.version());

    const char* two_char_ptr = reinterpret_cast<const char*>(two_blob.data());
    std::vector<float> two_float_vec(two_blob.size() / sizeof(float));
    for (size_t i = 0; i < two_float_vec.size(); i++) {
        two_float_vec[i] = *reinterpret_cast<const float*>(two_char_ptr + i * sizeof(float));
    }
    std::cout << "vector two size: " << two_float_vec.size() << std::endl;

    Feature ft2(std::move(two_float_vec), std::string(two.model()), two.version());

    if (one_float_vec.size() != two_float_vec.size()) {
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
