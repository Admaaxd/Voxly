#include "Chunk.h"

Chunk::Chunk(glm::vec3 position) : offset(position) {
    generateChunk();
}

void Chunk::generateChunk() {
    blocks.clear();

    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetFractalOctaves(1);
    noise.SetFractalLacunarity(3.0f);
    noise.SetFractalGain(0.5f);

    constexpr GLfloat noiseScale = 0.9f;

    // Use a 1D array instead of a 3D vector
    chunkData.resize(CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE, BlockType::AIR);

    // Generate terrain height using noise
    for (int16_t x = 0; x < CHUNK_SIZE; ++x) {
        for (int16_t z = 0; z < CHUNK_SIZE; ++z) {
            GLfloat noiseValue = noise.GetNoise((float)(x + offset.x) * noiseScale, (float)(z + offset.z) * noiseScale);
            int16_t height = static_cast<int>((noiseValue + 1.0f) * 0.6f * CHUNK_HEIGHT);

            for (int16_t y = 0; y <= height; ++y) {
                getBlock(x, y, z) = BlockType::SOLID;
            }
        }
    }

    // Generate visible blocks with correct faces
    for (int16_t x = 0; x < CHUNK_SIZE; x++) {
        for (int16_t y = 0; y < CHUNK_HEIGHT; y++) {
            for (int16_t z = 0; z < CHUNK_SIZE; z++) {
                if (getBlock(x, y, z) == BlockType::SOLID) {
                    bool left = (x == 0) || (getBlock(x - 1, y, z) == BlockType::AIR);
                    bool right = (x == CHUNK_SIZE - 1) || (getBlock(x + 1, y, z) == BlockType::AIR);
                    bool bottom = (y == 0) || (getBlock(x, y - 1, z) == BlockType::AIR);
                    bool top = (y == CHUNK_HEIGHT - 1) || (getBlock(x, y + 1, z) == BlockType::AIR);
                    bool back = (z == 0) || (getBlock(x, y, z - 1) == BlockType::AIR);
                    bool front = (z == CHUNK_SIZE - 1) || (getBlock(x, y, z + 1) == BlockType::AIR);

                    if (left || right || bottom || top || back || front) {
                        blocks.emplace_back(glm::vec3(x, y, z) + offset, BlockType::SOLID);
                    }
                }
            }
        }
    }
}

void Chunk::render(shader& shader) {
    for (Block& block : blocks) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), block.position);
        shader.setMat4("model", model);
        block.render();
    }
}

inline BlockType& Chunk::getBlock(int x, int y, int z)
{
    return chunkData[x + CHUNK_SIZE * (y + CHUNK_HEIGHT * z)];
}
