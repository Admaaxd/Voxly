#include "World.h"

World::World() {
    for (int x = -renderDistance; x <= renderDistance; ++x) {
        for (int z = -renderDistance; z <= renderDistance; ++z) {
            glm::vec3 position(x * CHUNK_SIZE, 0, z * CHUNK_SIZE);
            chunks.emplace(std::make_pair(x, z), Chunk(position));
        }
    }
}

void World::render(shader& mainShader) {
    for (auto& [key, chunk] : chunks) {
        chunk.render(mainShader);
    }
}