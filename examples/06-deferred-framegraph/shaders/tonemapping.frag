#version 450

#include "lib/color.glsl"

layout(location = 0) in vec2 vTexCoords;

layout(location = 0) out vec3 FragColor;

layout(binding = 0) uniform sampler2D texture0;

void main() {
    const vec4 source = texture(texture0, vTexCoords);

    vec3 color = toneMapACES(source.rgb);
    color = linearToGamma(color);

    FragColor = color;
}