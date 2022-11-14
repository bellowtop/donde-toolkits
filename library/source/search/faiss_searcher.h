#include "definitions.h"
#include "nlohmann/json.hpp"
#include "search/api.h"
#include "search/driver.h"

#include <map>

using namespace std;

using json = nlohmann::json;

namespace search {
class FaissSearcher : public Searcher {

  public:
    FaissSearcher(const json& config, Driver& driver);
    ~FaissSearcher() = default;

    RetCode Init() override {
        spdlog::warn("Init is not implemented by BruteForceSearch");
        return RetCode::RET_OK;
    };

    RetCode Terminate() override {
        spdlog::warn("Iterminate is not implemented by BruteForceSearch");
        return RetCode::RET_OK;
    };

    RetCode TrainIndex() override {
        spdlog::warn("TrainIndex is not implemented by BruteForceSearch");
        return RetCode::RET_OK;
    };

    std::vector<FeatureSearchItem> SearchFeature(const std::string& db_id, const Feature& query,
                                                 size_t topk) override;

    std::vector<std::string> AddFeatures(const std::string& db_id,
                                         const std::vector<FeatureDbItem>& features) override;

    RetCode RemoveFeatures(const std::string& db_id,
                           const std::vector<std::string>& feature_ids) override;

  private:
    json _config;
    Driver& _driver;
};

} // namespace search
