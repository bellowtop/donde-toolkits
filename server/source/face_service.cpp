#include "face_service.h"

#include "Poco/Format.h"
#include "Poco/Timestamp.h"
#include "concurrent_processor.h"
#include "config.h"
#include "detector_worker.h"
#include "face_pipeline.h"
#include "nlohmann/json.hpp"
#include "pb/server.grpc.pb.h"
#include "pb/server.pb.h"
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

using Poco::format;
using Poco::Logger;
using Poco::Timestamp;

using namespace std;

using com::sekirocc::face_service::BatchDetectRequest;
using com::sekirocc::face_service::BatchDetectResponse;
using com::sekirocc::face_service::FaceService;

using com::sekirocc::face_service::FaceDetectRequest;
using com::sekirocc::face_service::FaceDetectResponse;
// using com::sekirocc::face_service::StatusCode;

using com::sekirocc::face_service::BoundingPoly;
using com::sekirocc::face_service::FaceFeature;
using com::sekirocc::face_service::FaceObject;
using com::sekirocc::face_service::ObjectInfo;
using com::sekirocc::face_service::Vertex;

using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;

using json = nlohmann::json;

FaceServiceImpl::FaceServiceImpl(Config& server_config, Logger& parent)
    : config(server_config),
      logger(Logger::get(parent.name() + ".FaceServiceImpl")),
      device_id(server_config.get_device_id()),
      pipeline(config.get_pipeline_config(), device_id, logger){};

void FaceServiceImpl::Start() {
    const json& conf = pipeline.GetConfig();
    int concurrent = 4;

    auto detectorProcessor = std::make_shared<ConcurrentProcessor<DetectorWorker>>(
        conf, concurrent, device_id, logger);
    pipeline.Init(detectorProcessor, detectorProcessor, detectorProcessor, detectorProcessor);
};

void FaceServiceImpl::Stop() { pipeline.Terminate(); };

Status FaceServiceImpl::BatchDetect(ServerContext* context, const BatchDetectRequest* request,
                                    BatchDetectResponse* response) {

    int count = request->requests_size();
    std::vector<Frame*> frames;
    frames.reserve(count);

    if (count == 0) {
        return Status(StatusCode::INVALID_ARGUMENT, "requests size is 0");
    }

    for (int i = 0; i < count; i++) {
        const FaceDetectRequest& req = request->requests(i);
        if (!req.has_image()) {
            return Status(StatusCode::INVALID_ARGUMENT, "invalid request image", "image is null");
        }
        const std::string& image_data = req.image().data();
        const std::vector<uint8_t> image_char_vec(image_data.begin(), image_data.end());
        Frame* frame = pipeline.Decode(image_char_vec);
        frames.push_back(frame);
    }
    DetectResult* ret = pipeline.Detect(*frames[0]);

    logger.debug("pipeline.Detect DetectResult: %s", ret);

    return Status::OK;
}

// Status FaceServiceImpl::BatchDetect(ServerContext *context,
//                                            const BatchDetectRequest *request,
//                                            BatchDetectResponse *response) {
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
//         std::vector<Feature> fts = pipeline.BatchDetect(frames);
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
