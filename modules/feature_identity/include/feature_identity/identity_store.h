#include "donde/definitions.h"

#include <cstddef>
#include <unordered_map>
#include <vector>

namespace donde_toolkits::feature_identity {

//
// IdentityStore is a small in-memory database for feature search, can only handle small amount of
// features, up limit 1000k?
// typically for local devices or apps.
//
class IdentityStore {
  public:
    IdentityStore(size_t db_size, std::string feature_model_, int feature_version_,
                  int feature_dimension_);

    ~IdentityStore(){};

    RetCode LoadFromFile(const std::string& db_filepath);

    RetCode AddFeature(const Feature& feature);

    std::vector<Feature> SearchFeature(const Feature& target, int topK);

  private:
    void load() {}

  private:
    std::string db_persist_filepath_;

    // TODO use k-v db?
    std::unordered_map<std::string, Feature> db_store_;
    size_t db_size_;

    std::string feature_model_;
    int feature_version_;
    int feature_dimension_;
};
} // namespace donde_toolkits::feature_identity
