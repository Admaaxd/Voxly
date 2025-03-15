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
