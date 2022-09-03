#include "feature_search.h"

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

using com::sekirocc::face_service::AddFeatureRequest;
using com::sekirocc::face_service::AddFeatureResponse;
using com::sekirocc::face_service::DeleteFeatureRequest;
using com::sekirocc::face_service::DeleteFeatureResponse;
using com::sekirocc::face_service::SearchFeatureRequest;
using com::sekirocc::face_service::SearchFeatureResponse;
using com::sekirocc::face_service::TrainIndexRequest;
using com::sekirocc::face_service::TrainIndexResponse;

using grpc::ServerContext;
using grpc::Status;

using json = nlohmann::json;

FeatureSearchImpl::FeatureSearchImpl(Config& server_config)
    : config(server_config), device_id(server_config.get_device_id()){};

FeatureSearchImpl::~FeatureSearchImpl(){};

void FeatureSearchImpl::Start(){};

void FeatureSearchImpl::Stop(){};

Status FeatureSearchImpl::TrainIndex(ServerContext* context, const TrainIndexRequest* request,
                                     TrainIndexResponse* response) {
    return Status::OK;
};

Status FeatureSearchImpl::AddFeature(ServerContext* context, const AddFeatureRequest* request,
                                     AddFeatureResponse* response) {
    return Status::OK;
};

Status FeatureSearchImpl::DeleteFeature(ServerContext* context, const DeleteFeatureRequest* request,
                                        DeleteFeatureResponse* response) {
    return Status::OK;
};

Status FeatureSearchImpl::SearchFeature(ServerContext* context, const SearchFeatureRequest* request,
                                        SearchFeatureResponse* response) {
    return Status::OK;
};
