#version 430 core

struct CompactBlockVertex {
    uint position;
    uint normal;
    uint texCoord;
};

vec3 unpackPosition(uint data, vec3 chunkOffset) {
    float x = float(data & 0x3FF);
    float y = float((data >> 10) & 0x3FF);
    float z = float((data >> 20) & 0x3FF);
    return vec3(x, y, z) + chunkOffset;
}

vec3 unpackNormal(uint data) {
    float scale = 1023.0;
    return normalize(vec3(float((data & 0x3FF)) / scale - 0.5,
                          float(((data >> 10) & 0x3FF)) / scale - 0.5,
                          float(((data >> 20) & 0x3FF)) / scale - 0.5));
}

vec2 unpackTexCoord(uint data) {
    return vec2(unpackHalf2x16(data).x, unpackHalf2x16(data).y);
}

layout(std430, binding = 0) buffer VertexBuffer {
    CompactBlockVertex vertices[];
};

layout(std430, binding = 1) buffer IndexBuffer {
    uint indices[];
};

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform vec3 cameraPos;

uniform vec3 chunkOffset;

out vec3 FragPos;
out vec2 TexCoord;
out vec3 Normal;
out float visibility;

const float fogDensity = 0.005;
const float fogGradient = 1.5;

void main() {
    uint index = indices[gl_VertexID];
    uint packedPosition = vertices[index].position;

    vec3 pos = unpackPosition(packedPosition, chunkOffset);

    vec3 normal = unpackNormal(vertices[index].normal);
    vec2 texCoord = unpackTexCoord(vertices[index].texCoord);

    FragPos = pos;
    Normal = normal;
    TexCoord = texCoord;

    // Calculate fog visibility
    float distance = length(pos - cameraPos);
    float fogFactor = distance * fogDensity;
    visibility = exp(-pow(fogFactor, fogGradient));
    visibility = clamp(visibility, 0.0, 1.0);

    gl_Position = projection * view * model * vec4(pos, 1.0);
}
