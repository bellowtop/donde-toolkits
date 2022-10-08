#pragma once

#include "Poco/Format.h"
#include "Poco/Logger.h"
#include "Poco/Timestamp.h"
#include "config.h"
#include "face_pipeline.h"
#include "gen/pb-cpp/server.grpc.pb.h"
#include "search/searcher.h"

// #include "spdlog/spdlog.h"

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <iostream>

using namespace std;

using com::sekirocc::face_service::AddFeatureRequest;
using com::sekirocc::face_service::AddFeatureResponse;
using com::sekirocc::face_service::DeleteFeatureRequest;
using com::sekirocc::face_service::DeleteFeatureResponse;
using com::sekirocc::face_service::FeatureSearch;
using com::sekirocc::face_service::SearchFeatureRequest;
using com::sekirocc::face_service::SearchFeatureResponse;
using com::sekirocc::face_service::TrainIndexRequest;
using com::sekirocc::face_service::TrainIndexResponse;

using grpc::ServerContext;
using grpc::Status;

class FeatureSearchImpl final : public FeatureSearch::Service {
  public:
    FeatureSearchImpl(Config& server_config);
    ~FeatureSearchImpl();

    void Start();
    void Stop();

    Status TrainIndex(ServerContext* context, const TrainIndexRequest* request,
                      TrainIndexResponse* response) override;

    Status AddFeature(ServerContext* context, const AddFeatureRequest* request,
                      AddFeatureResponse* response) override;

    Status DeleteFeature(ServerContext* context, const DeleteFeatureRequest* request,
                         DeleteFeatureResponse* response) override;

    Status SearchFeature(ServerContext* context, const SearchFeatureRequest* request,
                         SearchFeatureResponse* response) override;

  private:
    Config& config;
    std::shared_ptr<search::Searcher> searcher;
    // spdlog::Logger& logger;
};
