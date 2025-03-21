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
    std::vector<std::reference_wrapper<Chunk>> getChunks();

    bool hasChunk(const std::pair<int, int>& cpos);

    Chunk* getChunkPtr(const std::pair<int, int>& cpos);

    void loadChunk(int16_t x, int16_t z);
    void unloadChunk(int16_t x, int16_t z);
    void updateChunks(glm::vec3 playerPosition);

    void processMeshUploads();  
private:
    constexpr static int16_t renderDistance = 10;
    std::unordered_map<std::pair<int, int>, Chunk, hash_pair> chunks;
    ThreadPool threadPool;
    std::mutex meshQueueMutex;
    std::mutex chunksMutex;
    std::queue<ChunkMeshData> meshUploadQueue;
    ChunkMeshData generateChunkMeshData(std::pair<int16_t, int16_t> chunkCoord, glm::vec3 position);
    std::deque<std::pair<int, int>> pendingChunks;
    std::mutex pendingMutex;

    std::chrono::steady_clock::time_point lastChunkLoadTime;
    std::chrono::milliseconds loadDelay = std::chrono::milliseconds(10);
};