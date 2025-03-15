#version 430 core

// Define the compact vertex structure matching the CPU layout.
struct CompactBlockVertex {
    uint packedPosition;
    uint packedNormal;
    uint packedTexCoord;
};

layout(std430, binding = 0) buffer VertexBuffer {
    CompactBlockVertex vertices[];
};

layout(std430, binding = 1) buffer IndexBuffer {
    uint indices[];
};

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

// Unpack position: Convert from 10-bit components to original range [-0.5, 0.5].
vec3 unpackPosition(uint packedPos) {
    uint x = (packedPos >>  0) & 0x3FF;
    uint y = (packedPos >> 10) & 0x3FF;
    uint z = (packedPos >> 20) & 0x3FF;
    float xf = float(x) / 1023.0 - 0.5;
    float yf = float(y) / 1023.0 - 0.5;
    float zf = float(z) / 1023.0 - 0.5;
    return vec3(xf, yf, zf);
}

vec3 unpackNormal(uint packedNormal) {
    float x = float((packedNormal >>  0) & 0x3FF) / 1023.0;
    float y = float((packedNormal >> 10) & 0x3FF) / 1023.0;
    float z = float((packedNormal >> 20) & 0x3FF) / 1023.0;
    return normalize(vec3(x, y, z) * 2.0 - 1.0);
}

vec2 unpackTexCoord(uint packedTexCoord) {
    return unpackHalf2x16(packedTexCoord);
}

void main() {
    uint vertexIndex = indices[gl_VertexID];
    CompactBlockVertex cv = vertices[vertexIndex];

    vec3 pos = unpackPosition(cv.packedPosition);
    vec3 norm = unpackNormal(cv.packedNormal);
    vec2 tex = unpackTexCoord(cv.packedTexCoord);

    FragPos = vec3(model * vec4(pos, 1.0));
    Normal = mat3(transpose(inverse(model))) * norm;
    TexCoord = tex;

    gl_Position = projection * view * model * vec4(pos, 1.0);
}
