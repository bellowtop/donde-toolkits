#include "SQLiteCpp/SQLiteCpp.h"
#include "donde/definitions.h"
#include "donde/feature_search/api.h"
#include "donde/feature_search/definitions.h"
#include "donde/utils.h"
#include "fmt/format.h"
#include "nlohmann/json.hpp"

#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <ios>
#include <iterator>
#include <map>
#include <memory>
#include <msgpack.hpp>
#include <nlohmann/json_fwd.hpp>
#include <opencv2/core/hal/interface.h>
#include <sqlite3.h>
#include <sstream>
#include <stdexcept>
using namespace std;

using donde::feature_search::FeatureSearchComp;
using donde::feature_search::FeatureSearchItem;

namespace donde {

class FeatureTopkRanking {
  public:
    FeatureTopkRanking(const Feature& query, int topk) : _query(query), _topk(topk){};

    void FeedIn(const Feature& ft) {
        float score = ft.compare(_query);
        if (_min_heap.size() < _topk) {
            _min_heap.emplace(ft, score);
        }
        if (_min_heap.top().score < score) {
            _min_heap.pop();
            _min_heap.emplace(ft, score);
        }
    };

    void FeedIn(const std::vector<Feature>& fts) {
        for (const auto& ft : fts) {
            FeedIn(ft);
        }
    };

    void FeedIn(const std::vector<FeatureSearchItem> items) {
        for (auto& item : items) {
            if (_min_heap.size() < _topk) {
                _min_heap.emplace(item.target, item.score);
                continue;
            }
            if (_min_heap.top().score < item.score) {
                _min_heap.pop();
                _min_heap.emplace(item.target, item.score);
            }
        }
    };

    std::vector<FeatureSearchItem> SortOut() {
        std::vector<FeatureSearchItem> ret;
        ret.reserve(_min_heap.size());

        while (!_min_heap.empty()) {
            auto& item = _min_heap.top();
            _min_heap.pop();
            ret.emplace_back(item.target, item.score);
        }

        std::reverse(ret.begin(), ret.end());

        return ret;
    };

  private:
    const Feature& _query;
    const int _topk;
    std::priority_queue<FeatureSearchItem, std::vector<FeatureSearchItem>, FeatureSearchComp>
        _min_heap;
};

} // namespace donde
