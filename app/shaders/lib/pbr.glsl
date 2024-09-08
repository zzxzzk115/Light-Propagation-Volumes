#ifndef PBR_GLSL
#define PBR_GLSL

#include "math.glsl"

// Fresnel Schlick approximation for specular reflection
vec3 FresnelSchlick(float cosTheta, vec3 F0) {
    // Fresnel term approximated by Schlick's method, using the base reflectivity (F0)
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// GGX/Trowbridge-Reitz normal distribution function (NDF) for microfacet distribution
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom + 1e-12; // Add small epsilon to prevent division by zero

    return num / denom; // GGX distribution result
}

// GTR (Generalized Trowbridge-Reitz) distribution function with an additional gamma parameter
float DistributionGTR(vec3 N, vec3 H, float roughness, float gamma) {
    float alpha = roughness * roughness;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float alpha2 = alpha * alpha;

    float denom = (NdotH2 * (alpha2 - 1.0) + 1.0);
    denom = PI * pow(denom, gamma);

    float c = alpha2;
    float D = c / (denom + 1e-12); // Add epsilon to avoid division by zero

    return D;
}

// Schlick-GGX geometry function for self-shadowing (visibility term)
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0; // GGX geometry term approximation

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k + 1e-12; // Add epsilon to prevent division by zero

    return num / denom; // Geometry function result
}

// Smith's method for geometric shadowing (GGX model), using Schlick-GGX geometry
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2; // Combined geometry term
}

// GGX visibility function (Smith Joint Visibility), using separate NdotV and NdotL
float V_GGX(float NdotV, float NdotL, float a) {
    const float a2 = a * a;
    const float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - a2) + a2);
    const float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - a2) + a2);
    const float GGX = GGXV + GGXL;

    return GGX > 0.0 ? 0.5 / GGX : 0.0;
}

// GGX visibility function with correlated terms for roughness
float V_SmithGGXCorrelated(float NdotV, float NdotL, float roughness) {
    const float a2 = pow(roughness, 4.0); // Roughness squared, for correlated GGX
    const float GGXV = NdotL * sqrt(NdotV * NdotV * (1.0 - a2) + a2);
    const float GGXL = NdotV * sqrt(NdotL * NdotL * (1.0 - a2) + a2);
    return 0.5 / (GGXV + GGXL + 1e-12); // Return correlated GGX visibility term
}

#endif
