#version 450

layout(location = 0) in vec3 iPosition;
layout(location = 1) in vec3 iColor;

layout(location = 0) out vec3 vFragColor;

void main() {
    gl_Position = vec4(iPosition, 1.0);
    vFragColor = iColor;
}