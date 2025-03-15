#pragma once

#include "Block.h"
#include "shader.h"
#include "FastNoiseLite.h"
#include <vector>

constexpr int32_t CHUNK_SIZE = 16;                  // Number of blocks along x, z
constexpr int32_t CHUNK_HEIGHT = 64;                // Number of blocks along y

class Chunk {
private:
    std::vector<BlockType> chunkData;
    std::vector<Block> blocks;
    glm::vec3 offset;
    FastNoiseLite noise;

public:
    Chunk(glm::vec3 position);
    void generateChunk();
    void render(shader& shader);

    inline BlockType& getBlock(int x, int y, int z);
};
