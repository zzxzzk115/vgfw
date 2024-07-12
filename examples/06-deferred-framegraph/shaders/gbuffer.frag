#version 450

layout(location = 0) in vec2 vTexCoords;
layout(location = 1) in vec3 vFragPos;
layout(location = 2) in mat3 vTBN;

layout(location = 0) out vec3 gPosition;
layout(location = 1) out vec3 gNormal;
layout(location = 2) out vec3 gAlbedo;
layout(location = 3) out vec3 gEmissive;
layout(location = 4) out vec3 gMetallicRoughnessAO;

layout(binding = 1) uniform PrimitiveMaterial {
    int baseColorTextureIndex;
    int metallicRoughnessTextureIndex;
    int normalTextureIndex;
    int occlusionTextureIndex;
    int emissiveTextureIndex;
} uMaterial;

layout(binding = 0) uniform sampler2D pbrTextures[5];

void main() {
    vec3 baseColor;
    float alpha = 1.0;
    if(uMaterial.baseColorTextureIndex != -1) {
        vec4 color = texture(pbrTextures[uMaterial.baseColorTextureIndex], vTexCoords);
        baseColor = color.rgb;
        alpha = color.a;
    }

    if(alpha < 0.5) {
        discard;
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

    gPosition = vFragPos;
    gNormal = normal;
    gAlbedo = baseColor;
    gEmissive = emissive;
    gMetallicRoughnessAO = vec3(metallic, roughness, ao);
}