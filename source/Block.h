#pragma once

#include <glad/glad.h>
#include <glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <gtc/packing.hpp>
#include <vector>

struct CompactBlockVertex
{
    uint32_t packedPosition;
    uint32_t packedNormal;
    uint32_t packedTexCoord;
};

enum class BlockType {
    AIR, SOLID
};

class Block {
private:
    GLuint VAO;
    GLuint vertexSSBO; // SSBO for vertex data
    GLuint indexSSBO;  // SSBO for index data

public:
    glm::vec3 position;
    BlockType type;

    Block(glm::vec3 pos, BlockType blockType);
    void setupMesh();   // Setup VAO, VBO, EBO
    void render();      // Render block
};
