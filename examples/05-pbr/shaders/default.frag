#version 450

#include "lib/lib.glsl"

layout(location = 0) in vec2 vTexCoords;
layout(location = 1) in vec3 vFragPos;
layout(location = 2) in vec3 vNormal;
layout(location = 3) in vec4 vTangent;

layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform PrimitiveMaterial {
    int baseColorTextureIndex;
    int metallicRoughnessTextureIndex;
    int normalTextureIndex;
    int occlusionTextureIndex;
    int emissiveTextureIndex;
} uMaterial;

layout(binding = 1) uniform sampler2D pbrTextures[5];

void main() {
    vec2 uv = vec2(vTexCoords.x, 1.0 - vTexCoords.y);
    FragColor = vec4(texture(pbrTextures[uMaterial.baseColorTextureIndex], uv).rgb, 1.0);
}