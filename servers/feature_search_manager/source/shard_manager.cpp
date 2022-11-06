#include "shard_manager.h"

Shard* ShardManager::FindOrCreateWritableShard(std::string db_id) { return {}; };

RetCode ShardManager::LoadShards() { return {}; };

// FIXME why shared_ptr? what the purpose of this api?
std::vector<std::shared_ptr<search::DBItem>> ShardManager::ListUserDBs() { return {}; };

// FIXME why poiter? what the purpose of this api?
std::vector<Shard*> ShardManager::ListShards(std::string db_id) { return {}; };
