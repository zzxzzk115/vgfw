#version 450

#include "lib/lib.glsl"

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

layout(location = 0) out vec2 vTexCoords;
layout(location = 1) out vec3 vFragPos;
layout(location = 2) out vec3 vNormal;

layout(location = 0) uniform mat4 view;
layout(location = 1) uniform mat4 projection;

void main()
{
    gl_Position = projection * view * vec4(aPos, 1.0);
    vTexCoords = aTexCoords;
    vFragPos = aPos;
    vNormal = aNormal;
}