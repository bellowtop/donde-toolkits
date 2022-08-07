#pragma once

#include "Poco/Format.h"
#include "Poco/Logger.h"
#include "Poco/Timestamp.h"
#include "config.h"
#include "face_pipeline.h"
#include "pb/server.grpc.pb.h"

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <iostream>

using Poco::format;
using Poco::Logger;
using Poco::Timestamp;

using namespace std;

using com::sekirocc::face_service::BatchDetectRequest;
using com::sekirocc::face_service::BatchDetectResponse;
using com::sekirocc::face_service::FaceService;

using grpc::ServerContext;
using grpc::Status;

class FaceServiceImpl final : public FaceService::Service {
  public:
    FaceServiceImpl(Config& server_config, Logger& parent);

    void Start();
    void Stop();

    Status BatchDetect(ServerContext* context, const BatchDetectRequest* request,
                       BatchDetectResponse* response) override;

  private:
    Config& config;
    Poco::Logger& logger;
    std::string device_id;
    FacePipeline pipeline;
};
