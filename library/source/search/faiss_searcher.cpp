#include "nlohmann/json.hpp"
#include "search/db_searcher.h"
#include "search/driver.h"
#include "types.h"

#include <map>

using namespace std;

using json = nlohmann::json;

namespace search {
    class FaissSearcher : public Searcher {

      public:
        FaissSearcher(const json& config);
        ~FaissSearcher() = default;

        RetCode TrainIndex() override;

        std::vector<FeatureSearchItem> Search(const Feature& query, size_t topk) override;

        std::vector<std::string> AddFeatures(const std::vector<FeatureDbItem>& features) override;

        RetCode RemoveFeatures(const std::vector<std::string>& feature_ids) override;

      private:
        json _config;
    };

    FaissSearcher::FaissSearcher(const json& config) : _config(config){};

} // namespace search
