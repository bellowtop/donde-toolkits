#pragma once

#include "Poco/Format.h"
#include "Poco/Logger.h"
#include "Poco/Timestamp.h"
#include "config.h"
#include "face_pipeline.h"
#include "gen/pb-cpp/server.grpc.pb.h"

// #include "spdlog/spdlog.h"

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <iostream>

using namespace std;

using com::sekirocc::face_service::DetectionRequest;
using com::sekirocc::face_service::DetectionResponse;
using com::sekirocc::face_service::ExtractionRequest;
using com::sekirocc::face_service::ExtractionResponse;
using com::sekirocc::face_service::FaceService;

using grpc::ServerContext;
using grpc::Status;

class FaceServiceImpl final : public FaceService::Service {
  public:
    FaceServiceImpl(Config& server_config);

    void Start();
    void Stop();

    Status Detect(ServerContext* context, const DetectionRequest* request,
                  DetectionResponse* response) override;

    Status ExtractFeature(ServerContext* context, const ExtractionRequest* request,
                          ExtractionResponse* response) override;


  private:
    Config& config;
    // spdlog::Logger& logger;
    std::string device_id;
    FacePipeline pipeline;
};
