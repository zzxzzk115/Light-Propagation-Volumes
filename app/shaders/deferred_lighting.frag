#version 460 core

#include "lib/pbr.glsl"
#include "lib/lpv.glsl"
#include "lib/csm.glsl"
#include "lib/depth.glsl"

layout(location = 0) in vec2 vTexCoords;

layout(location = 0) out vec3 FragColor;

layout(binding = 0) uniform Camera {
    vec3 position;
    mat4 view;
    mat4 projection;
    mat4 inverseView;
    mat4 inverseProjection;
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
layout(binding = 5) uniform sampler2D SceneDepth;
layout(binding = 6) uniform sampler2DArrayShadow CascadedShadowMaps;
layout(binding = 7) uniform sampler3D Propagated_SH_R;
layout(binding = 8) uniform sampler3D Propagated_SH_G;
layout(binding = 9) uniform sampler3D Propagated_SH_B;
layout(binding = 10) uniform sampler2D HBAO;

uniform uint uVisualMode;
uniform mat4 uLightVP;
uniform RadianceInjection uInjection;

struct Settings {
    int enableHBAO;
    int enableSSR;
    int enableTAA;
    int enableBloom;
};
uniform Settings uSettings;

void main() {
    const float depth = getDepth(SceneDepth, vTexCoords);
    if (depth >= 1.0) discard;
    
    vec3 fragPos = texture(gPosition, vTexCoords).rgb;
    vec3 normal = texture(gNormal, vTexCoords).rgb;
    vec3 baseColor = texture(gAlbedo, vTexCoords).rgb;
    vec3 emissive = texture(gEmissive, vTexCoords).rgb;
    vec4 metallicRoughnessAO = texture(gMetallicRoughnessAO, vTexCoords);
    float metallic = metallicRoughnessAO.r;
    float roughness = metallicRoughnessAO.g;
    float ao = metallicRoughnessAO.b;

    if (uSettings.enableHBAO == 1) {
        ao *= texture(HBAO, vTexCoords).r;
    }

    // select cascade layer
    vec3 fragPosViewSpace = viewPositionFromDepth(depth, vTexCoords, uCamera.inverseProjection);
    uint cascadeIndex = selectCascadeIndex(fragPosViewSpace);

    // calculate shadow
    float lightVisibility = getLightVisibility(cascadeIndex, fragPos, CascadedShadowMaps);

    vec3 V = normalize(uCamera.position - fragPos);
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
    vec3 Lo_Diffuse = kD * baseColor / PI * lightRadiance * NdotL * lightVisibility;
    vec3 Lo_Specular = specular * lightRadiance * NdotL * lightVisibility;

    // Indirect light (LPV-based GI)
    const vec3 cellCoords = (fragPos - uInjection.gridAABBMin) / uInjection.gridCellSize / uInjection.gridSize;

    // clang-format off
    const SH_Coefficients coeffs = {
    texture(Propagated_SH_R, cellCoords, 0), texture(Propagated_SH_G, cellCoords, 0), texture(Propagated_SH_B, cellCoords, 0), };
    // clang-format on

    const vec4 SH_Intensity = SH_Evaluate(-normal);
    const vec3 LPV_Intensity = vec3(dot(SH_Intensity, coeffs.red), dot(SH_Intensity, coeffs.green), dot(SH_Intensity, coeffs.blue));

    const vec3 radiance = max(LPV_Intensity * 4 / uInjection.gridCellSize / uInjection.gridCellSize, 0.0);
    if(uVisualMode != 1) {
        Lo_Diffuse += baseColor * radiance;
    }

    Lo_Diffuse *= ao;

    if(uVisualMode != 2) {
        FragColor = Lo_Diffuse + Lo_Specular + emissive;
    } else {
        FragColor = radiance;
    }
}