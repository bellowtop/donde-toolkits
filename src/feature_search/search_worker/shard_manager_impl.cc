#include "shard_manager_impl.h"

#include "donde/definitions.h"
#include "donde/utils.h"
#include "feature_search/search_manager/shard_impl.h"

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
        return {};
    }
    return iter->second;
};

RetCode ShardManagerImpl::ManageShard(std::string db_id, Shard* shard) {
    if (shard->GetShardInfo().db_id != db_id) {
        spdlog::warn("shard info.db_id[{}] differ from input db_id[{}]",
                     shard->GetShardInfo().db_id, db_id);
        return RetCode::RET_ERR;
    }

    auto iter = _db_shards.find(db_id);
    if (iter == _db_shards.end()) {
        _db_shards[db_id] = std::vector<Shard*>();
    }

    for (auto& it : _db_shards[db_id]) {
        if (it->GetShardID() == shard->GetShardID()) {
            spdlog::warn("shard [{}] is already managed in db[{}]", shard->GetShardID(), db_id);
            return RetCode::RET_OK;
        }
    };
    _db_shards[db_id].push_back(shard);

    return {};
};

RetCode ShardManagerImpl::CloseShard(std::string db_id, std::string shard_id) {
    _driver.CloseShard(db_id, shard_id);
    return {};
};

Shard* ShardManagerImpl::FindShard(std::string db_id) {
    auto shards = ListShards(db_id);
    for (auto& s : shards) {
        if (!s->IsClosed()) {
            return s;
        }
    }
    return nullptr;
};

} // namespace search_worker
} // namespace feature_search
} // namespace donde
