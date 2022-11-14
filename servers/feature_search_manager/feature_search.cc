#include "feature_search.h"

#include "Poco/Format.h"
#include "Poco/Timestamp.h"
#include "api/common.pb.h"
#include "api/feature_search.grpc.pb.h"
#include "api/feature_search.pb.h"
#include "config.h"
#include "donde/definitions.h"
#include "donde/feature_search/api.h"
#include "donde/utils.h"
#include "nlohmann/json.hpp"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

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

FeatureSearchManagerImpl::FeatureSearchManagerImpl(Config& server_config) : config(server_config){};

FeatureSearchManagerImpl::~FeatureSearchManagerImpl(){};

void FeatureSearchManagerImpl::Start() { searcher->Init(); };

void FeatureSearchManagerImpl::Stop() { searcher->Terminate(); };

Status FeatureSearchManagerImpl::DBNew(ServerContext* context, const DBNewRequest* request,
                                       DBNewResponse* response) {
    return Status::OK;
};

Status FeatureSearchManagerImpl::DBList(ServerContext* context, const DBListRequest* request,
                                        DBListResponse* response) {
    return Status::OK;
};

Status FeatureSearchManagerImpl::DBGet(ServerContext* context, const DBGetRequest* request,
                                       DBGetResponse* response) {
    return Status::OK;
};

Status FeatureSearchManagerImpl::DBDelete(ServerContext* context, const DBDeleteRequest* request,
                                          DBDeleteResponse* response) {
    return Status::OK;
};

Status FeatureSearchManagerImpl::AddFeature(ServerContext* context,
                                            const AddFeatureRequest* request,
                                            AddFeatureResponse* response) {
    auto db_id = request->db_id();

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
    std::vector<std::string> feature_ids = searcher->AddFeatures(db_id, fts);

    response->set_feature_id(feature_ids[0]);
    response->set_code(ResultCode::OK);

    return Status::OK;
};

Status FeatureSearchManagerImpl::DeleteFeature(ServerContext* context,
                                               const DeleteFeatureRequest* request,
                                               DeleteFeatureResponse* response) {

    auto db_id = request->db_id();
    std::string feature_id = request->feature_id();
    std::vector<std::string> feature_ids{feature_id};
    searcher->RemoveFeatures(db_id, feature_ids);

    return Status::OK;
};

Status FeatureSearchManagerImpl::SearchFeature(ServerContext* context,
                                               const SearchFeatureRequest* request,
                                               SearchFeatureResponse* response) {
    auto db_ids = request->db_ids();

    auto ft = request->query();
    auto topk = request->topk();
    Feature query(convertFeatureBlobToFloats(ft.blob()), std::string(ft.model()), ft.version());

    // FIXME: only search first db for now !!!
    std::vector<search::FeatureSearchItem> ret
        = searcher->SearchFeature(db_ids.Get(0), query, topk);

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
