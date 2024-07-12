#ifndef PBR_GLSL
#define PBR_GLSL

// Cook-Torrance GGX (Trowbridge-Reitz) Distribution
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.1415926535897932384626433832795 * denom * denom;

    return num / max(denom, 0.001); // prevent divide by zero for roughness=0.0 and NdotH=1.0
}

float GeometrySmith_GGX(float NdotX, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float num = NdotX;
    float denom = NdotX * (1.0 - a) + a;

    return num / denom;
}

// Smith's GGX Visibility Function (Schlick-Beckmann approximation)
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySmith_GGX(NdotV, roughness);
    float ggx1 = GeometrySmith_GGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Schlick's approximation for the Fresnel term
vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

#endif