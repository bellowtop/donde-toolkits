#pragma once

#include <vector>

using namespace std;

class Shard {};

class ShardManager {

  public:
    virtual Shard* AllocateShard() = 0;

  public:
    std::vector<Shard> shards;
};
