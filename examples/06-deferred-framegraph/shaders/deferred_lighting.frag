#version 450

#include "lib/pbr.glsl"

layout(location = 0) in vec2 vTexCoords;

layout(location = 0) out vec3 FragColor;

layout(binding = 0) uniform Camera {
    vec3 position;
    mat4 view;
    mat4 projection;
} uCamera;

layout(binding = 1) uniform DirectionalLight {
    vec3 direction;
    float intensity;
    vec3 color;
} uLight;

layout(binding = 0) uniform sampler2D gPosition;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D gAlbedo;
layout(binding = 3) uniform sampler2D gEmissive;
layout(binding = 4) uniform sampler2D gMetallicRoughnessAO;

void main() {
    vec3 fragPos = texture(gPosition, vTexCoords).rgb;
    vec3 normal = texture(gNormal, vTexCoords).rgb;
    vec3 baseColor = texture(gAlbedo, vTexCoords).rgb;
    vec3 emissive = texture(gEmissive, vTexCoords).rgb;
    vec4 metallicRoughnessAO = texture(gMetallicRoughnessAO, vTexCoords);
    float metallic = metallicRoughnessAO.r;
    float roughness = metallicRoughnessAO.g;
    float ao = metallicRoughnessAO.b;

    float lightIntensity = uLight.intensity;
    vec3 lightColor = uLight.color;
    vec3 lightDir = -uLight.direction;

    // Ambient
    vec3 ambient = lightIntensity * lightColor * 0.02;

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = lightIntensity * diff * lightColor;

    // Specular (Cook-Torrance BRDF)
    vec3 viewDir = normalize(uCamera.position - fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float NDF = DistributionGGX(normal, halfwayDir, roughness);
    float G = GeometrySmith(normal, viewDir, lightDir, roughness);
    vec3 F0 = vec3(0.04); // default specular reflectance
    vec3 F = FresnelSchlick(max(dot(halfwayDir, viewDir), 0.0), F0);
    vec3 specular = (NDF * G * F) / (4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0));

    // Combine ambient, diffuse, and specular components
    FragColor = (ambient + (1.0 - metallic) * diffuse + metallic * specular) * baseColor * ao + emissive;
}