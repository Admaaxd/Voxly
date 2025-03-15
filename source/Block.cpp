#include "Block.h"

GLfloat cubeVertices[] = {
    // Positions          // Normals            // Texture Coords

    // Front Face (CCW order)
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,

    // Back Face (CCW order)
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,

     // Left Face (CCW order)
     -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
     -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

     // Right Face (CCW order)
      0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
      0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
      0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
      0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,

      // Bottom Face (CCW order)
     -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
     -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
      0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
      0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,

      // Top Face (CCW order)
     -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
      0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
      0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
     -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
};

GLuint indices[] = {
    0, 1, 2, 2, 3, 0,  // Front face
    4, 5, 6, 6, 7, 4,  // Back face
    8, 9, 10, 10, 11, 8,  // Left face
    12, 13, 14, 14, 15, 12,  // Right face
    16, 17, 18, 18, 19, 16,  // Bottom face
    20, 21, 22, 22, 23, 20   // Top face
};

Block::Block(glm::vec3 pos, BlockType blockType)
    : position(pos), type(blockType), VAO(0), vertexSSBO(0), indexSSBO(0)
{
    setupMesh();
}

uint32_t packPosition(glm::vec3 pos) {
    // Shift the range: -0.5 -> 0, 0.5 -> 1.0, then scale to [0,1023]
    float scale = 1023.0f;
    uint32_t x = static_cast<uint32_t>(std::round((pos.x + 0.5f) * scale)) & 0x3FF; // 0x3FF = 1023
    uint32_t y = static_cast<uint32_t>(std::round((pos.y + 0.5f) * scale)) & 0x3FF;
    uint32_t z = static_cast<uint32_t>(std::round((pos.z + 0.5f) * scale)) & 0x3FF;
    // Pack x into bits 0-9, y into 10-19, z into 20-29.
    return (z << 20) | (y << 10) | (x);
}

uint32_t packNormal(glm::vec3 normal) {
    normal = glm::normalize(normal);
    glm::vec3 n = (normal * 0.5f + 0.5f) * 1023.0f; // Now in [0,1023]
    uint32_t nx = static_cast<uint32_t>(std::round(n.x)) & 0x3FF;
    uint32_t ny = static_cast<uint32_t>(std::round(n.y)) & 0x3FF;
    uint32_t nz = static_cast<uint32_t>(std::round(n.z)) & 0x3FF;
    return (nz << 20) | (ny << 10) | (nx);
}

uint32_t packTexCoord(glm::vec2 texCoord) {
    uint16_t u = glm::packHalf1x16(texCoord.x);
    uint16_t v = glm::packHalf1x16(texCoord.y);
    return (static_cast<uint32_t>(u) << 16) | static_cast<uint32_t>(v);
}

void Block::setupMesh() 
{
    std::vector<CompactBlockVertex> compactVertices;
    compactVertices.reserve(24);

    // Loop through the 24 vertices from cubeVertices array.
    for (int i = 0; i < 24; i++) {
        CompactBlockVertex cv;
        glm::vec3 pos(
            cubeVertices[i * 8 + 0],
            cubeVertices[i * 8 + 1],
            cubeVertices[i * 8 + 2]
        );
        glm::vec3 norm(
            cubeVertices[i * 8 + 3],
            cubeVertices[i * 8 + 4],
            cubeVertices[i * 8 + 5]
        );
        glm::vec2 tex(
            cubeVertices[i * 8 + 6],
            cubeVertices[i * 8 + 7]
        );

        // Pack each attribute.
        cv.packedPosition = packPosition(pos);
        cv.packedNormal = packNormal(norm);
        cv.packedTexCoord = packTexCoord(tex);

        compactVertices.push_back(cv);
    }

    // Create and upload the vertex SSBO.
    glGenBuffers(1, &vertexSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, vertexSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
        compactVertices.size() * sizeof(CompactBlockVertex),
        compactVertices.data(),
        GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, vertexSSBO);

    // Create and upload the index SSBO.
    glGenBuffers(1, &indexSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, indexSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, indexSSBO);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    glGenVertexArrays(1, &VAO);
}

void Block::render() {
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}
