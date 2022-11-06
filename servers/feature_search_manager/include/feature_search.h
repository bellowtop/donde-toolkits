#pragma once

#include "Poco/Format.h"
#include "Poco/Logger.h"
#include "Poco/Timestamp.h"
#include "config.h"
#include "gen/pb-cpp/feature_search.grpc.pb.h"
#include "search/db_searcher.h"
#include "shard_manager.h"
#include "worker_client.h"

// #include "spdlog/spdlog.h"

#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <iostream>
#include <unordered_map>

using namespace std;

using com::sekirocc::feature_search::DBNewRequest;
using com::sekirocc::feature_search::DBNewResponse;

using com::sekirocc::feature_search::DBListRequest;
using com::sekirocc::feature_search::DBListResponse;

using com::sekirocc::feature_search::DBDeleteRequest;
using com::sekirocc::feature_search::DBDeleteResponse;

using com::sekirocc::feature_search::DBGetRequest;
using com::sekirocc::feature_search::DBGetResponse;

using com::sekirocc::feature_search::AddFeatureRequest;
using com::sekirocc::feature_search::AddFeatureResponse;
using com::sekirocc::feature_search::DeleteFeatureRequest;
using com::sekirocc::feature_search::DeleteFeatureResponse;
using com::sekirocc::feature_search::FeatureSearchManager;
using com::sekirocc::feature_search::SearchFeatureRequest;
using com::sekirocc::feature_search::SearchFeatureResponse;

using grpc::ServerContext;
using grpc::Status;

class FeatureSearchManagerImpl final : public FeatureSearchManager::Service {
  public:
    FeatureSearchManagerImpl(Config& server_config);
    ~FeatureSearchManagerImpl();

    void Start();
    void Stop();

    Status DBNew(ServerContext* context, const DBNewRequest* request,
                 DBNewResponse* response) override;

    Status DBList(ServerContext* context, const DBListRequest* request,
                  DBListResponse* response) override;

    Status DBDelete(ServerContext* context, const DBDeleteRequest* request,
                    DBDeleteResponse* response) override;

    Status DBGet(ServerContext* context, const DBGetRequest* request,
                 DBGetResponse* response) override;

    Status AddFeature(ServerContext* context, const AddFeatureRequest* request,
                      AddFeatureResponse* response) override;

    Status DeleteFeature(ServerContext* context, const DeleteFeatureRequest* request,
                         DeleteFeatureResponse* response) override;

    Status SearchFeature(ServerContext* context, const SearchFeatureRequest* request,
                         SearchFeatureResponse* response) override;

  private:
    Config& config;
    // spdlog::Logger& logger;

    std::shared_ptr<search::Searcher> searcher;

    std::vector<std::string> db_ids;

    // worker_id => worker
    std::unordered_map<std::string, WorkerClient*> workers;

    // worker_id => db_ids
    std::unordered_map<std::string, std::vector<std::string>> worker_dbs;
};
