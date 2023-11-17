#include "donde/feature_identity/identity_store.h"

namespace donde_toolkits::feature_identity {

//// public apis

IdentityStore::IdentityStore(size_t db_size, std::string feature_model, int feature_version,
                             int feature_dimension)
    : db_size_(db_size),
      feature_model_(feature_model),
      feature_version_(feature_version),
      feature_dimension_(feature_dimension) {}

RetCode IdentityStore::LoadFromFile(const std::string& db_filepath) {
    db_persist_filepath_ = db_filepath;
    return {};
}

RetCode IdentityStore::AddFeature(const Feature& feature) { return {}; }

std::vector<Feature> IdentityStore::SearchFeature(const Feature& target, int topK) { return {}; }

//// private apis

} // namespace donde_toolkits::feature_identity
