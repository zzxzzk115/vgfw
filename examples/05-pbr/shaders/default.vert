#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec4 aTangent;

layout(location = 0) out vec2 vTexCoords;
layout(location = 1) out vec3 vFragPos;
layout(location = 2) out mat3 vTBN;

layout(binding = 0) uniform Camera {
    vec3 position;
    mat4 view;
    mat4 projection;
} uCamera;

void main() {
    gl_Position = uCamera.projection * uCamera.view * vec4(aPos, 1.0);
    vTexCoords = aTexCoords;
    vFragPos = aPos;
    vTBN = mat3(aTangent.xyz, cross(aTangent.xyz, aNormal) * aTangent.w, aNormal);
}