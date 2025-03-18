#include "World.h"

World::World() {
    for (int16_t x = -renderDistance; x <= renderDistance; ++x) {
        for (int16_t z = -renderDistance; z <= renderDistance; ++z) {
            glm::vec3 position(x * (CHUNK_SIZE - 1), 0, z * (CHUNK_SIZE - 1));
            chunks.emplace(std::make_pair(x, z), Chunk(position, std::make_pair(x, z), this));
        }
    }
}

void World::render(shader& mainShader) {
    for (auto& [key, chunk] : chunks) {
        mainShader.setVec3("chunkOffset", chunk.getOffset());
        chunk.render(mainShader);
    }
}

std::vector<std::reference_wrapper<Chunk>> World::getChunks()
{
    std::vector<std::reference_wrapper<Chunk>> chunkList;
    for (auto& [key, chunk] : chunks) {
        chunkList.push_back(chunk);
    }
    return chunkList;
}

bool World::hasChunk(const std::pair<int, int>& cpos) const {
    return (chunks.find(cpos) != chunks.end());
}

Chunk* World::getChunkPtr(const std::pair<int, int>& cpos) {
    auto it = chunks.find(cpos);
    if (it != chunks.end()) {
        return &it->second;
    }
    return nullptr;
}

void World::loadChunk(int16_t x, int16_t z) {
    std::pair<int, int> chunkCoord = { x, z };

    if (hasChunk(chunkCoord)) {
        return;
    }

    glm::vec3 position(x * (CHUNK_SIZE - 1), 0, z * (CHUNK_SIZE - 1));

    chunks.emplace(chunkCoord, Chunk(position, chunkCoord, this));
}

void World::unloadChunk(int16_t x, int16_t z) {
    std::pair<int, int> chunkCoord = { x, z };

    auto it = chunks.find(chunkCoord);
    if (it != chunks.end()) {
        it->second.cleanupOpenGLResources();
        chunks.erase(it);
    }
}

void World::updateChunks(glm::vec3 playerPosition) {
    int16_t playerChunkX = static_cast<int16_t>(std::floor(playerPosition.x / CHUNK_SIZE));
    int16_t playerChunkZ = static_cast<int16_t>(std::floor(playerPosition.z / CHUNK_SIZE));

    std::unordered_set<std::pair<int, int>, hash_pair> activeChunks;

    for (int16_t x = -renderDistance; x <= renderDistance; ++x) {
        for (int16_t z = -renderDistance; z <= renderDistance; ++z) {
            std::pair<int, int> chunkCoord = { playerChunkX + x, playerChunkZ + z };
            activeChunks.insert(chunkCoord);

            if (!hasChunk(chunkCoord)) {
                loadChunk(chunkCoord.first, chunkCoord.second);
            }
        }
    }

    std::vector<std::pair<int, int>> chunksToUnload;
    for (const auto& [chunkCoord, chunk] : chunks) {
        if (activeChunks.find(chunkCoord) == activeChunks.end()) {
            chunksToUnload.push_back(chunkCoord);
        }
    }

    for (const auto& chunkCoord : chunksToUnload) {
        unloadChunk(chunkCoord.first, chunkCoord.second);
    }
}
