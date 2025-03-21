#include "Chunk.h"
#include "World.h"

Chunk::Chunk(glm::vec3 worldPos, std::pair<int, int> chunkCoord, World* worldRef) : coord(chunkCoord), offset(worldPos), world(worldRef) 
{
    glGenVertexArrays(1, &dummyVAO);
}

Chunk::~Chunk() {}

void Chunk::cleanupOpenGLResources()
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Delete SSBOs
    if (glIsBuffer(vertexSSBO)) {
        glDeleteBuffers(1, &vertexSSBO);
        vertexSSBO = 0;
    }
    if (glIsBuffer(indexSSBO)) {
        glDeleteBuffers(1, &indexSSBO);
        indexSSBO = 0;
    }

    // Delete VAO
    if (glIsVertexArray(dummyVAO)) {
        glDeleteVertexArrays(1, &dummyVAO);
        dummyVAO = 0;
    }
}

void Chunk::generateChunk() {
    chunkData.resize(CHUNK_SIZE * CHUNK_HEIGHT * CHUNK_SIZE, BlockType::AIR);

    noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
    noise.SetFractalOctaves(1);
    noise.SetFractalLacunarity(1.0f);
    noise.SetFractalGain(0.5f);
    constexpr GLfloat noiseScale = 0.9f;

    for (int32_t x = 0; x < CHUNK_SIZE; ++x) {
        for (int32_t z = 0; z < CHUNK_SIZE; ++z) {
            GLfloat worldX = offset.x + x; // The absolute world x
            GLfloat worldZ = offset.z + z; // The absolute world z

            GLfloat noiseValue = noise.GetNoise(worldX * noiseScale, worldZ * noiseScale);
            int32_t height = static_cast<int>((noiseValue + 1.0f) * 0.5f * CHUNK_HEIGHT);

            for (int32_t y = 0; y <= height; ++y) {
                getBlock(x, y, z) = BlockType::SOLID;
            }
        }
    }
}

void Chunk::generateMesh_CPUOnly() {
    compactVertices.clear();
    indices.clear();

    GLuint indexOffset = 0;

    for (int32_t x = 0; x < CHUNK_SIZE; ++x) {
        for (int32_t y = 0; y < CHUNK_HEIGHT; ++y) {
            for (int32_t z = 0; z < CHUNK_SIZE; ++z) {
                if (getBlock(x, y, z) == BlockType::AIR) continue;

                glm::vec3 blockPos = glm::vec3(x, y, z);

                bool left = isFaceVisible(x - 1, y, z);
                bool right = isFaceVisible(x + 1, y, z);
                bool bottom = isFaceVisible(x, y - 1, z);
                bool top = isFaceVisible(x, y + 1, z);
                bool back = isFaceVisible(x, y, z - 1);
                bool front = isFaceVisible(x, y, z + 1);
            }
        }
    }
}

void Chunk::generateMesh() {
   
    generateMesh_CPUOnly();
    uploadMeshToGPU();
}

bool Chunk::isFaceVisible(int x, int y, int z) {
    if (x < 0) {
        auto neighborCoord = std::make_pair(coord.first - 1, coord.second);
        if (world->hasChunk(neighborCoord)) {
            Chunk* neighbor = world->getChunkPtr(neighborCoord);
            return (neighbor->getBlock(x + CHUNK_SIZE, y, z) == BlockType::AIR);
        }
        else {
            return true;
        }
    }
    if (x >= CHUNK_SIZE) {
        auto neighborCoord = std::make_pair(coord.first + 1, coord.second);
        if (world->hasChunk(neighborCoord)) {
            Chunk* neighbor = world->getChunkPtr(neighborCoord);
            return (neighbor->getBlock(x - CHUNK_SIZE, y, z) == BlockType::AIR);
        }
        else {
            return true;
        }
    }

    if (z < 0) {
        auto neighborCoord = std::make_pair(coord.first, coord.second - 1);
        if (world->hasChunk(neighborCoord)) {
            Chunk* neighbor = world->getChunkPtr(neighborCoord);
            return (neighbor->getBlock(x, y, z + CHUNK_SIZE) == BlockType::AIR);
        }
        else {
            return true;
        }
    }
    if (z >= CHUNK_SIZE) {
        auto neighborCoord = std::make_pair(coord.first, coord.second + 1);
        if (world->hasChunk(neighborCoord)) {
            Chunk* neighbor = world->getChunkPtr(neighborCoord);
            return (neighbor->getBlock(x, y, z - CHUNK_SIZE) == BlockType::AIR);
        }
        else {
            return true;
        }
    }

    if (y < 0 || y >= CHUNK_HEIGHT) {
        return true;
    }

    return (getBlock(x, y, z) == BlockType::AIR);
}

void Chunk::uploadMeshToGPU() {
    glGenBuffers(1, &vertexSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, compactVertices.size() * sizeof(CompactBlockVertex), compactVertices.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertexSSBO);

    glGenBuffers(1, &indexSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, indexSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void Chunk::render(shader& shader) {
    if (!readyToRender)
        return;

    shader.use();
    glBindVertexArray(dummyVAO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertexSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, indexSSBO);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(indices.size()));
    glBindVertexArray(0);
}

inline BlockType& Chunk::getBlock(int x, int y, int z)
{
    return chunkData[x + CHUNK_SIZE * (y + CHUNK_HEIGHT * z)];
}

void Chunk::uploadMeshFromThread(const ChunkMeshData& mesh)
{
    compactVertices = mesh.vertices;
    indices = mesh.indices;

    uploadMeshToGPU();
    readyToRender = true;
}
