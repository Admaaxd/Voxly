#pragma once

#include "Block.h"
#include "shader.h"
#include "FastNoiseLite.h"
#include <vector>

constexpr int32_t CHUNK_SIZE = 16;                      // Number of blocks along x, z
constexpr int32_t CHUNK_HEIGHT = 128;                   // Number of blocks along y

class World;

struct ChunkMeshData {
	std::vector<CompactBlockVertex> vertices;
	std::vector<GLuint> indices;
    std::pair<int, int> coord;
    glm::vec3 offset;
    std::vector<BlockType> blocks;
};

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
    

    bool readyToRender = false;

public:
    Chunk(glm::vec3 position, std::pair<int, int> chunkCoord, World* worldRef);
    ~Chunk();
    void cleanupOpenGLResources();
    void generateChunk();
    void generateMesh();
    void render(shader& shader);
    void uploadMeshToGPU();
    bool isFaceVisible(int x, int y, int z);
    inline BlockType& getBlock(int x, int y, int z);
    glm::vec3 getOffset() const { return offset; }
    void uploadMeshFromThread(const ChunkMeshData& mesh);
    void generateMesh_CPUOnly();

    std::pair<int, int> coord;
};
