#pragma once

#include "Block.h"
#include "shader.h"
#include "FastNoiseLite.h"
#include <vector>

constexpr int32_t CHUNK_SIZE = 16;                  // Number of blocks along x, z
constexpr int32_t CHUNK_HEIGHT = 128;                // Number of blocks along y

class World;

class Chunk {
private:
    std::vector<BlockType> chunkData;
    glm::vec3 offset;
    FastNoiseLite noise;

    std::vector<CompactBlockVertex> compactVertices;
    std::vector<GLuint> indices;
    GLuint VAO, vertexSSBO, indexSSBO;

    GLuint dummyVAO;

    World* world;
    std::pair<int, int> coord;

public:
    Chunk(glm::vec3 position, std::pair<int, int> chunkCoord, World* worldRef);
    void generateChunk();
    void generateMesh();
    void render(shader& shader);
    void uploadMeshToGPU();
    bool isFaceVisible(int x, int y, int z);
    void AddFace(std::vector<CompactBlockVertex>& compactVertices, std::vector<GLuint>& indices, glm::vec3 pos, const GLfloat* faceData, GLuint& indexOffset);
    inline BlockType& getBlock(int x, int y, int z);
    glm::vec3 getOffset() const { return offset; }
};
