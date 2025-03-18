#include "Chunk.h"
#include "World.h"

Chunk::Chunk(glm::vec3 worldPos, std::pair<int, int> chunkCoord, World* worldRef) : coord(chunkCoord), offset(worldPos), world(worldRef) {
    glGenVertexArrays(1, &dummyVAO);
    generateChunk();
    generateMesh();
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

void Chunk::generateMesh() {
    compactVertices.clear();
    indices.clear();

    GLuint indexOffset = 0;

    for (int32_t x = 0; x < CHUNK_SIZE; ++x) {
        for (int32_t y = 0; y < CHUNK_HEIGHT; ++y) {
            for (int32_t z = 0; z < CHUNK_SIZE; ++z) {
                if (getBlock(x, y, z) == BlockType::AIR) continue;

                glm::vec3 blockPos = glm::vec3(x, y, z);

                // Determine visible faces, considering chunk borders
                bool left = isFaceVisible(x - 1, y, z);
                bool right = isFaceVisible(x + 1, y, z);
                bool bottom = isFaceVisible(x, y - 1, z);
                bool top = isFaceVisible(x, y + 1, z);
                bool back = isFaceVisible(x, y, z - 1);
                bool front = isFaceVisible(x, y, z + 1);

                if (left)   AddFace(compactVertices, indices, blockPos, LEFT_FACE, indexOffset);
                if (right)  AddFace(compactVertices, indices, blockPos, RIGHT_FACE, indexOffset);
                if (bottom) AddFace(compactVertices, indices, blockPos, BOTTOM_FACE, indexOffset);
                if (top)    AddFace(compactVertices, indices, blockPos, TOP_FACE, indexOffset);
                if (back)   AddFace(compactVertices, indices, blockPos, BACK_FACE, indexOffset);
                if (front)  AddFace(compactVertices, indices, blockPos, FRONT_FACE, indexOffset);
            }
        }
    }

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

uint32_t packTexCoord(glm::vec2 texCoord)
{
    uint16_t u = glm::packHalf1x16(texCoord.x);
    uint16_t v = glm::packHalf1x16(texCoord.y);
    return (static_cast<uint32_t>(u) << 16) | static_cast<uint32_t>(v);
}

uint32_t packNormal(glm::vec3 normal)
{
    normal = glm::normalize(normal);
    glm::vec3 n = (normal * 0.5f + 0.5f) * 1023.0f; // [0,1023]
    uint32_t nx = static_cast<uint32_t>(std::round(n.x)) & 0x3FF;
    uint32_t ny = static_cast<uint32_t>(std::round(n.y)) & 0x3FF;
    uint32_t nz = static_cast<uint32_t>(std::round(n.z)) & 0x3FF;
    return (nz << 20) | (ny << 10) | (nx);
}

uint32_t packPosition(glm::vec3 pos) {
    // Directly store the block position within the chunk
    uint32_t x = static_cast<uint32_t>(pos.x) & 0x3FF;
    uint32_t y = static_cast<uint32_t>(pos.y) & 0x3FF;
    uint32_t z = static_cast<uint32_t>(pos.z) & 0x3FF;

    return (z << 20) | (y << 10) | (x);
}

void Chunk::AddFace(std::vector<CompactBlockVertex>& compactVertices, std::vector<GLuint>& indices, glm::vec3 pos, const GLfloat* faceData, GLuint& indexOffset) {

    for (int i = 0; i < 4; i++) {
        CompactBlockVertex vertex;
        vertex.position = packPosition(pos + glm::vec3(faceData[i * 8 + 0], faceData[i * 8 + 1], faceData[i * 8 + 2]));
        vertex.normal = packNormal(glm::vec3(faceData[i * 8 + 3], faceData[i * 8 + 4], faceData[i * 8 + 5]));
        vertex.texCoord = packTexCoord(glm::vec2(faceData[i * 8 + 6], faceData[i * 8 + 7]));

        compactVertices.push_back(vertex);
    }

    indices.insert(indices.end(), { indexOffset, indexOffset + 1, indexOffset + 2, indexOffset, indexOffset + 2, indexOffset + 3 });
    indexOffset += 4;
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
