#pragma once

#include "Poco/Format.h"
#include "Poco/Logger.h"
#include "Poco/Timestamp.h"
#include "api/feature_extract.grpc.pb.h"
#include "api/feature_extract.pb.h"
#include "config.h"
#include "source/pipeline/face_pipeline.h"

// #include "spdlog/spdlog.h"

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <iostream>

using namespace std;

using com::sekirocc::feature_extract::CompareRequest;
using com::sekirocc::feature_extract::CompareResponse;
using com::sekirocc::feature_extract::DetectionRequest;
using com::sekirocc::feature_extract::DetectionResponse;
using com::sekirocc::feature_extract::ExtractionRequest;
using com::sekirocc::feature_extract::ExtractionResponse;
using com::sekirocc::feature_extract::FaceService;

using grpc::ServerContext;
using grpc::Status;

class FaceServiceImpl final : public FaceService::Service {
  public:
    FaceServiceImpl(Config& server_config);
    ~FaceServiceImpl();

    void Start();
    void Stop();

    Status Detect(ServerContext* context, const DetectionRequest* request,
                  DetectionResponse* response) override;

    Status ExtractFeature(ServerContext* context, const ExtractionRequest* request,
                          ExtractionResponse* response) override;

    Status CompareFeature(ServerContext* context, const CompareRequest* request,
                          CompareResponse* response) override;

  private:
    Config& config;
    // spdlog::Logger& logger;
    std::string device_id;
    FacePipeline pipeline;
};
