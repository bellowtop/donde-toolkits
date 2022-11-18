#include "shard_manager_impl.h"

#include "donde/utils.h"
#include "source/feature_search/search_manager/shard_impl.h"

#include <spdlog/spdlog.h>

namespace donde {
namespace feature_search {
namespace search_worker {

////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// ShardManagerImpl
////////////////////////////////////////////////////////////////////////////////////////////////////////////

ShardManagerImpl::ShardManagerImpl(Driver& driver) : ShardManager(driver), _driver(driver){};

std::vector<Shard*> ShardManagerImpl::ListShards(std::string db_id) {
    auto iter = _db_shards.find(db_id);
    if (iter == _db_shards.end()) {
        throw "db_id not exists";
    }
    return iter->second;
};

RetCode ShardManagerImpl::ManageShard(std::string db_id, Shard*) { return {}; };

RetCode ShardManagerImpl::CloseShard(std::string db_id, std::string shard_id) {
    _driver.CloseShard(db_id, shard_id);
    return {};
};

} // namespace search_worker
} // namespace feature_search
} // namespace donde
