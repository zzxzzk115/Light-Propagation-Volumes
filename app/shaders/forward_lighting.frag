#version 460 core

#include "lib/pbr.glsl"
#include "lib/lpv.glsl"

layout(location = 0) in vec2 vTexCoords;
layout(location = 1) in vec3 vFragPos;
layout(location = 2) in mat3 vTBN;

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

layout(binding = 2) uniform PrimitiveMaterial {
    int baseColorTextureIndex;
    int metallicRoughnessTextureIndex;
    int normalTextureIndex;
    int occlusionTextureIndex;
    int emissiveTextureIndex;
} uMaterial;

layout(binding = 0) uniform sampler3D Propagated_SH_R;
layout(binding = 1) uniform sampler3D Propagated_SH_G;
layout(binding = 2) uniform sampler3D Propagated_SH_B;
layout(binding = 3) uniform sampler2D PBR_Textures[5];

uniform uint uVisualMode;
uniform RadianceInjection uInjection;

void main() {
    vec3 baseColor;
    float alpha = 1.0;
    if(uMaterial.baseColorTextureIndex != -1) {
        vec4 color = texture(PBR_Textures[uMaterial.baseColorTextureIndex], vTexCoords);
        baseColor = color.rgb;
        alpha = color.a;
    }

    if(alpha < 0.5) {
        discard;
    }

    float metallic = 0.0;
    float roughness = 0.5;
    if(uMaterial.metallicRoughnessTextureIndex != -1) {
        vec4 metallicRoughness = texture(PBR_Textures[uMaterial.metallicRoughnessTextureIndex], vTexCoords);
        metallic = metallicRoughness.b;
        roughness = metallicRoughness.g;
    }

    vec3 normal = normalize(vTBN[2]);
    if(uMaterial.normalTextureIndex != -1) {
        vec3 normalColor = texture(PBR_Textures[uMaterial.normalTextureIndex], vTexCoords).rgb;
        vec3 tangentNormal = normalColor * 2.0 - 1.0;
        normal = tangentNormal * transpose(vTBN);
    }

    float ao = 1.0;
    if(uMaterial.occlusionTextureIndex != -1) {
        ao = texture(PBR_Textures[uMaterial.occlusionTextureIndex], vTexCoords).r;
    }

    vec3 emissive;
    if(uMaterial.emissiveTextureIndex != -1) {
        emissive = texture(PBR_Textures[uMaterial.emissiveTextureIndex], vTexCoords).rgb;
    }

    vec3 V = normalize(uCamera.position - vFragPos);
    vec3 N = normal;
    vec3 L = normalize(-uLight.direction);
    vec3 H = normalize(V + L);
    vec3 lightRadiance = uLight.color * uLight.intensity;

    vec3 F0 = vec3(0.04);
    F0 = mix(F0, baseColor, metallic);

    // cook-torrance brdf
    float gamma = 2.0;
    float NDF = DistributionGTR(N, H, roughness, gamma);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - metallic;

    vec3 nominator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 1e-12;
    vec3 specular = nominator / denominator;

    float NdotL = max(dot(N, L), 0.0);

    // Direct light
    vec3 Lo_Diffuse = kD * baseColor / PI * lightRadiance * NdotL;
    vec3 Lo_Specular = specular * lightRadiance * NdotL;

    // Indirect light (LPV-based GI)
    const vec3 cellCoords = (vFragPos - uInjection.gridAABBMin) / uInjection.gridCellSize / uInjection.gridSize;

    // clang-format off
    const SH_Coefficients coeffs = {
        texture(Propagated_SH_R, cellCoords, 0),
        texture(Propagated_SH_G, cellCoords, 0),
        texture(Propagated_SH_B, cellCoords, 0),
    };
    // clang-format on

    const vec4 SH_Intensity = SH_Evaluate(-normal);
    const vec3 LPV_Intensity = vec3(dot(SH_Intensity, coeffs.red), dot(SH_Intensity, coeffs.green), dot(SH_Intensity, coeffs.blue));

    const vec3 radiance = max(LPV_Intensity * 4 / uInjection.gridCellSize / uInjection.gridCellSize, 0.0);
    if(uVisualMode != 1) {
        Lo_Diffuse += baseColor * radiance * ao;
    }

    if(uVisualMode != 2) {
        FragColor = Lo_Diffuse + Lo_Specular + emissive;
    } else {
        FragColor = radiance;
    }
}