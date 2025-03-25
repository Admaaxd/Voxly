#include "Chunk.h"
#include "World.h"

Chunk::Chunk(glm::vec3 worldPos, std::pair<int, int> chunkCoord, World* worldRef) : VAO(0), indexSSBO(0), 
                                                            vertexSSBO(0), coord(chunkCoord), offset(worldPos), world(worldRef) 
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

inline BlockType& Chunk::getBlock(int16_t x, int16_t y, int16_t z)
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
