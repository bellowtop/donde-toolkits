#include "face_service.h"

#include "Poco/Format.h"
#include "Poco/Timestamp.h"
#include "concurrent_processor.h"
#include "config.h"
#include "processor_worker.h"
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
#include <iostream>
#include <iterator>
#include <memory>
#include <sstream>
#include <string>

using namespace std;

using com::sekirocc::face_service::DetectionRequest;
using com::sekirocc::face_service::DetectionResponse;

using com::sekirocc::face_service::FaceService;

using com::sekirocc::face_service::Rect;
using com::sekirocc::face_service::ResultCode;

using com::sekirocc::face_service::FaceObject;

using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;

using json = nlohmann::json;

FaceServiceImpl::FaceServiceImpl(Config& server_config)
    : config(server_config),
      device_id(server_config.get_device_id()),
      pipeline(config.get_pipeline_config(), device_id){};

void FaceServiceImpl::Start() {
    const json& conf = pipeline.GetConfig();
    int concurrent = conf.value("concurrent", 1);

    auto detectorProcessor
        = std::make_shared<ConcurrentProcessor<DetectorWorker>>(conf, concurrent, device_id);
    auto landmarksProcessor
        = std::make_shared<ConcurrentProcessor<LandmarksWorker>>(conf, concurrent, device_id);
    pipeline.Init(detectorProcessor, landmarksProcessor, detectorProcessor, detectorProcessor);
};

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

    for (auto& detected_face : result->faces) {
        // fill response
        FaceObject* face_obj = response->add_faces();
        face_obj->set_code(ResultCode::OK);
        face_obj->set_quality(0.0f); // TODO quality unimplemented

        Rect* rect = face_obj->mutable_rectangle();
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

// Status FaceServiceImpl::BatchDetection(ServerContext *context,
//                                            const DetectionRequest *request,
//                                            DetectionResponse *response) {
//
//         int count = request->requests_size();
//         std::vector<Frame> frames;
//         frames.reserve(count);
//
//         for (int i = 0; i < count; i++) {
//                 const FaceDetectRequest &req = request->requests(i);
//                 if (!req.has_image()) {
//                         return Status(StatusCode::INVALID_ARGUMENT, "invalid request image",
//                                       "image is null");
//                 }
//                 std::string const &image_data = req.image().data();
//                 Frame frame = pipeline.Decode(image_data);
//                 frames.push_back(frame);
//         }
//
//         std::vector<Feature> fts = pipeline.BatchDetection(frames);
//
//         for (int i = 0; i < count; i++) {
//                 FaceDetectResponse *resp = response->add_responses();
//                 Result *result = response->add_results();
//
//                 Feature ft = fts.at(i);
//                 if (!ft.IsValid()) {
//                         result->set_status(StatusCode::FACE_NOT_FOUND);
//                         continue;
//                 }
//                 result->set_status(StatusCode::OK);
//
//                 ObjectInfo *face_info = resp->add_face_infos();
//
//                 FaceFeature *f = face_info->mutable_feature();
//
//                 // int dim = ft.Dimension();
//                 // std::vector<float> raw_ft = ft.RawFeature();
//                 // char const *p = reinterpret_cast<char const *>(&raw_ft[0]);
//                 // std::string str;
//                 // str.resize(dim * sizeof(float));
//                 // std::copy(p, p + dim * sizeof(float), &str[0]);
//
//                 f->set_blob(str);
//                 f->set_version(ft.Version());
//
//                 // todo
//                 FaceObject *face = face_info->mutable_face();
//
//                 int landmarks_size = 5;
//                 for (int i = 0; i < landmarks_size; i++) {
//                         Vertex *vertex = face->add_landmarks();
//                 }
//                 BoundingPoly *bounding = face->mutable_rectangle();
//         }
//
//         return Status::OK;
// };
