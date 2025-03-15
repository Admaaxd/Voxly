#pragma once

#include <unordered_map>
#include <queue>
#include <map>
#include <unordered_set>
#include "Chunk.h"
#include "ThreadPool.h"

struct hash_pair {
    size_t operator()(const std::pair<int, int>& p) const {
        return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
    }
};

class World {
public:
    World();
    void render(shader& mainShader);
  
private:
    constexpr static int renderDistance = 1;
    std::unordered_map<std::pair<int, int>, Chunk, hash_pair> chunks;
};