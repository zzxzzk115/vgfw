#version 450

#include "lib/pbr.glsl"
#include "lib/color.glsl"

layout(location = 0) in vec2 vTexCoords;
layout(location = 1) in vec3 vFragPos;
layout(location = 2) in mat3 vTBN;

layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform PrimitiveMaterial {
    int baseColorTextureIndex;
    int metallicRoughnessTextureIndex;
    int normalTextureIndex;
    int occlusionTextureIndex;
    int emissiveTextureIndex;
} uMaterial;

layout(binding = 1) uniform DirectionalLight {
    vec3 direction;
    float intensity;
    vec3 color;
} uLight;

layout(binding = 2) uniform sampler2D pbrTextures[5];

layout(location = 2) uniform vec3 viewPos;

void main() {
    vec3 baseColor;
    if(uMaterial.baseColorTextureIndex != -1) {
        baseColor = texture(pbrTextures[uMaterial.baseColorTextureIndex], vTexCoords).rgb;
    }

    float metallic = 0.0;
    float roughness = 0.5;
    if(uMaterial.metallicRoughnessTextureIndex != -1) {
        vec4 metallicRoughness = texture(pbrTextures[uMaterial.metallicRoughnessTextureIndex], vTexCoords);
        metallic = metallicRoughness.b;
        roughness = metallicRoughness.g;
    }

    vec3 normal = normalize(vTBN[2]);
    if(uMaterial.normalTextureIndex != -1) {
        vec3 normalColor = texture(pbrTextures[uMaterial.normalTextureIndex], vTexCoords).rgb;
        vec3 tangentNormal = normalColor * 2.0 - 1.0;
        normal = tangentNormal * transpose(vTBN);
    }

    float ao = 1.0;
    if(uMaterial.occlusionTextureIndex != -1) {
        ao = texture(pbrTextures[uMaterial.occlusionTextureIndex], vTexCoords).r;
    }

    vec3 emissive;
    if(uMaterial.emissiveTextureIndex != -1) {
        emissive = texture(pbrTextures[uMaterial.emissiveTextureIndex], vTexCoords).rgb;
    }

    float lightIntensity = uLight.intensity;
    vec3 lightColor = uLight.color;
    vec3 lightDir = -uLight.direction;

    // Ambient
    vec3 ambient = lightIntensity * lightColor * 0.02;

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = lightIntensity * diff * lightColor;

    // Specular (Cook-Torrance BRDF)
    vec3 viewDir = normalize(viewPos - vFragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float NDF = DistributionGGX(normal, halfwayDir, roughness);
    float G = GeometrySmith(normal, viewDir, lightDir, roughness);
    vec3 F0 = vec3(0.04); // default specular reflectance
    vec3 F = FresnelSchlick(max(dot(halfwayDir, viewDir), 0.0), F0);
    vec3 specular = (NDF * G * F) / (4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0));

    // Combine ambient, diffuse, and specular components
    vec3 result = (ambient + (1.0 - metallic) * diffuse + metallic * specular) * baseColor * ao + emissive;

    // Tone-mapping
    result = toneMapACES(result);

    // Gamma correction
    result = linearToGamma(result);

    FragColor = vec4(result, 1.0);
}