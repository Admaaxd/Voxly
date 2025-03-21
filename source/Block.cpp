#include "Block.h"


uint32_t packPosition(glm::vec3 pos) {
    uint32_t x = static_cast<uint32_t>(pos.x) & 0x3FF;
    uint32_t y = static_cast<uint32_t>(pos.y) & 0x3FF;
    uint32_t z = static_cast<uint32_t>(pos.z) & 0x3FF;
    return (z << 20) | (y << 10) | (x);
}

uint32_t packNormal(glm::vec3 normal) {
    normal = glm::normalize(normal);
    glm::vec3 n = (normal * 0.5f + 0.5f) * 1023.0f;
    uint32_t nx = static_cast<uint32_t>(round(n.x)) & 0x3FF;
    uint32_t ny = static_cast<uint32_t>(round(n.y)) & 0x3FF;
    uint32_t nz = static_cast<uint32_t>(round(n.z)) & 0x3FF;
    return (nz << 20) | (ny << 10) | (nx);
}

uint32_t packTexCoord(glm::vec2 texCoord) {
    uint16_t u = glm::packHalf1x16(texCoord.x);
    uint16_t v = glm::packHalf1x16(texCoord.y);
    return (static_cast<uint32_t>(u) << 16) | static_cast<uint32_t>(v);
}

void AddFaceToMesh(std::vector<CompactBlockVertex>& compactVertices, std::vector<GLuint>& indices, glm::vec3 pos, const GLfloat* faceData, GLuint& indexOffset)
{
    for (int i = 0; i < 4; i++) {
        CompactBlockVertex vertex;
        vertex.position = packPosition(pos + glm::vec3(faceData[i * 8 + 0],
            faceData[i * 8 + 1],
            faceData[i * 8 + 2]));
        vertex.normal = packNormal(glm::vec3(faceData[i * 8 + 3],
            faceData[i * 8 + 4],
            faceData[i * 8 + 5]));
        vertex.texCoord = packTexCoord(glm::vec2(faceData[i * 8 + 6],
            faceData[i * 8 + 7]));
        compactVertices.push_back(vertex);
    }

    indices.insert(indices.end(), { indexOffset, indexOffset + 1,
                                      indexOffset + 2, indexOffset,
                                      indexOffset + 2, indexOffset + 3 });
    indexOffset += 4;
}
