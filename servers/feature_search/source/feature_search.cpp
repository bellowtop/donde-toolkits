#include "feature_search.h"

#include "Poco/Format.h"
#include "Poco/Timestamp.h"
#include "config.h"
#include "gen/pb-cpp/common.pb.h"
#include "gen/pb-cpp/feature_search.grpc.pb.h"
#include "gen/pb-cpp/feature_search.pb.h"
#include "nlohmann/json.hpp"
#include "search/searcher.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"
#include "types.h"
#include "utils.h"

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
#include <opencv2/core/hal/interface.h>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

using com::sekirocc::feature_search::AddFeatureRequest;

using com::sekirocc::feature_search::AddFeatureResponse;
using com::sekirocc::feature_search::DeleteFeatureRequest;
using com::sekirocc::feature_search::DeleteFeatureResponse;
using com::sekirocc::feature_search::SearchFeatureRequest;
using com::sekirocc::feature_search::SearchFeatureResponse;

using com::sekirocc::common::FaceFeature;

using com::sekirocc::common::ResultCode;

using grpc::ServerContext;
using grpc::Status;

using json = nlohmann::json;

FeatureSearchImpl::FeatureSearchImpl(Config& server_config)
    : config(server_config), searcher(new search::Searcher(server_config.get_searcher_config())){};

FeatureSearchImpl::~FeatureSearchImpl(){};

void FeatureSearchImpl::Start() { searcher->Init(); };

void FeatureSearchImpl::Stop() { searcher->Terminate(); };

Status FeatureSearchImpl::DBNew(ServerContext* context, const DBNewRequest* request,
                                DBNewResponse* response) {
    return Status::OK;
};

Status FeatureSearchImpl::DBList(ServerContext* context, const DBListRequest* request,
                                 DBListResponse* response) {
    return Status::OK;
};

Status FeatureSearchImpl::DBGet(ServerContext* context, const DBGetRequest* request,
                                DBGetResponse* response) {
    return Status::OK;
};

Status FeatureSearchImpl::DBDelete(ServerContext* context, const DBDeleteRequest* request,
                                   DBDeleteResponse* response) {
    return Status::OK;
};

Status FeatureSearchImpl::AddFeature(ServerContext* context, const AddFeatureRequest* request,
                                     AddFeatureResponse* response) {
    auto item = request->feature_item();
    auto ft = item.feature();

    // auto meta = ((AddFeatureRequest*)request)->mutable_meta();
    // (*meta)["a"] = "b";

    Feature feature(convertFeatureBlobToFloats(ft.blob()), std::string(ft.model()), ft.version());
    std::map<string, string> meta = convertMetadataToMap(item.meta());

    std::vector<search::FeatureDbItem> fts{search::FeatureDbItem{
        .feature = feature,
        .metadata = meta,
    }};
    std::vector<std::string> feature_ids = searcher->AddFeatures(fts);

    response->set_feature_id(feature_ids[0]);
    response->set_code(ResultCode::OK);

    return Status::OK;
};

Status FeatureSearchImpl::DeleteFeature(ServerContext* context, const DeleteFeatureRequest* request,
                                        DeleteFeatureResponse* response) {

    std::string feature_id = request->feature_id();
    std::vector<std::string> feature_ids{feature_id};
    searcher->RemoveFeatures(feature_ids);

    return Status::OK;
};

Status FeatureSearchImpl::SearchFeature(ServerContext* context, const SearchFeatureRequest* request,
                                        SearchFeatureResponse* response) {
    auto ft = request->query();
    auto topk = request->topk();
    Feature query(convertFeatureBlobToFloats(ft.blob()), std::string(ft.model()), ft.version());

    std::vector<search::FeatureSearchItem> ret = searcher->SearchFeature(query, topk);

    response->set_code(ResultCode::OK);

    for (const auto& feature_search_result : ret) {
        auto item = response->add_items();
        item->set_score(feature_search_result.score);

        // convert ft => face_feat.
        auto ft = feature_search_result.target;
        FaceFeature* face_feat = item->mutable_feature();

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
};
