#version 460 core

#include "lib/pbr.glsl"
#include "lib/lpv.glsl"
#include "lib/csm.glsl"
#include "lib/depth.glsl"

// Input texture coordinates from vertex shader
layout(location = 0) in vec2 vTexCoords;

// Output scene color and bright color (for bloom effect)
layout(location = 0) out vec3 SceneColor;
layout(location = 1) out vec3 SceneColorBright;

// Camera properties (position, view/projection matrices, etc.)
layout(binding = 0) uniform Camera {
    vec3 position;
    mat4 view;
    mat4 projection;
    mat4 inverseView;
    mat4 inverseProjection;
} uCamera;

// Directional light properties (direction, intensity, color)
layout(binding = 1) uniform DirectionalLight {
    vec3 direction;
    float intensity;
    vec3 color;
} uLight;

// G-buffer textures
layout(binding = 0) uniform sampler2D gNormal;                // Normal texture
layout(binding = 1) uniform sampler2D gAlbedo;                // Albedo (base color) texture
layout(binding = 2) uniform sampler2D gEmissive;              // Emissive texture
layout(binding = 3) uniform sampler2D gMetallicRoughnessAO;   // Metallic, Roughness, and AO (ambient occlusion) texture
layout(binding = 4) uniform sampler2D SceneDepth;             // Depth texture from G-buffer
layout(binding = 5) uniform sampler2DArrayShadow CascadedShadowMaps;  // Cascaded shadow maps
layout(binding = 6) uniform sampler3D Propagated_SH_R;        // Red SH (Spherical Harmonics) coefficients for LPV
layout(binding = 7) uniform sampler3D Propagated_SH_G;        // Green SH coefficients
layout(binding = 8) uniform sampler3D Propagated_SH_B;        // Blue SH coefficients
layout(binding = 9) uniform sampler2D HBAO;                   // Ambient occlusion map

// Light view-projection matrix for shadow calculation
uniform mat4 uLightVP;

// Radiance injection parameters (used for LPV grid)
uniform RadianceInjection uInjection;

// Settings for rendering features (e.g., HBAO, SSR, TAA, Bloom, visual mode)
struct Settings {
    int enableHBAO;        // Enable ambient occlusion
    int enableSSR;         // Enable screen-space reflections
    int enableTAA;         // Enable temporal anti-aliasing
    int enableBloom;       // Enable bloom effect
    uint visualMode;       // Visual debugging modes
};
uniform Settings uSettings;

void main() {
    // Retrieve depth from the scene's depth texture at the current fragment
    const float depth = getDepth(SceneDepth, vTexCoords);
    if (depth >= 1.0) discard;  // Discard fragment if it has no depth value

    // Convert depth to view space position
    vec3 fragPosViewSpace = viewPositionFromDepth(depth, vTexCoords, uCamera.inverseProjection);

    // Convert view space position to world space position
    vec3 fragPos = (uCamera.inverseView * vec4(fragPosViewSpace, 1.0)).xyz;

    // Sample normal, base color (albedo), emissive, and metallic/roughness/ao from G-buffer
    vec3 normal = texture(gNormal, vTexCoords).rgb;
    vec3 baseColor = texture(gAlbedo, vTexCoords).rgb;
    vec3 emissive = texture(gEmissive, vTexCoords).rgb;
    vec4 metallicRoughnessAO = texture(gMetallicRoughnessAO, vTexCoords);
    float metallic = metallicRoughnessAO.r;
    float roughness = metallicRoughnessAO.g;
    float ao = metallicRoughnessAO.b;

    // If HBAO (ambient occlusion) is enabled, multiply AO with the HBAO map value
    if (uSettings.enableHBAO == 1) {
        ao *= texture(HBAO, vTexCoords).r;
    }

    // Select the appropriate shadow cascade based on fragment view space position
    uint cascadeIndex = selectCascadeIndex(fragPosViewSpace);

    // Compute light visibility (shadow) using cascaded shadow maps
    float lightVisibility = getLightVisibility(cascadeIndex, fragPos, CascadedShadowMaps);

    // Camera view direction (from fragment to camera)
    vec3 V = normalize(uCamera.position - fragPos);

    // Normal and light direction
    vec3 N = normal;
    vec3 L = normalize(-uLight.direction);  // Light direction is negative because light shines in the opposite direction

    // Half-vector between light direction and view direction
    vec3 H = normalize(V + L);

    // Calculate the light radiance using its color and intensity
    vec3 lightRadiance = uLight.color * uLight.intensity;

    // Base reflectance (F0) for metallic surfaces, blended with base color based on metallic property
    vec3 F0 = vec3(0.04); // Default non-metallic reflectance
    F0 = mix(F0, baseColor, metallic); // Adjust F0 for metallic surfaces

    // Cook-Torrance BRDF components
    float gamma = 2.0;  // Gamma for GTR distribution
    float NDF = DistributionGTR(N, H, roughness, gamma);  // Normal distribution function (GTR)
    float G = GeometrySmith(N, V, L, roughness);          // Geometry (shadowing/masking)
    vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);     // Fresnel term using Schlick approximation

    // Split reflectance into specular (kS) and diffuse (kD) components
    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;  // Diffuse component
    kD *= 1.0 - metallic;      // Diffuse only for non-metallic surfaces

    // Compute the Cook-Torrance BRDF for specular reflection
    vec3 nominator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 1e-12;  // Prevent division by zero
    vec3 specular = nominator / denominator;

    // Lambertian diffuse reflection term
    float NdotL = max(dot(N, L), 0.0);

    // Direct lighting components (diffuse and specular)
    vec3 Lo_Diffuse = kD * baseColor / PI * lightRadiance * NdotL * lightVisibility;
    vec3 Lo_Specular = specular * lightRadiance * NdotL * lightVisibility;

    // Indirect lighting via LPV (Light Propagation Volumes) for global illumination
    const vec3 cellCoords = (fragPos - uInjection.gridAABBMin) / uInjection.gridCellSize / uInjection.gridSize;

    // Retrieve propagated SH coefficients for each color channel from the 3D texture
    const SH_Coefficients coeffs = {
        texture(Propagated_SH_R, cellCoords, 0),
        texture(Propagated_SH_G, cellCoords, 0),
        texture(Propagated_SH_B, cellCoords, 0)
    };

    // Evaluate SH lighting for the normal direction
    const vec4 SH_Intensity = SH_Evaluate(-normal);
    const vec3 LPV_Intensity = vec3(dot(SH_Intensity, coeffs.red), dot(SH_Intensity, coeffs.green), dot(SH_Intensity, coeffs.blue));

    // Scale the LPV intensity based on grid cell size
    const vec3 radiance = max(LPV_Intensity * 4 / uInjection.gridCellSize / uInjection.gridCellSize, 0.0);

    // Add indirect lighting based on LPV if not in debug visual mode
    if(uSettings.visualMode != 1) {
        Lo_Diffuse += baseColor * radiance;
    }

    // Multiply final diffuse result by ambient occlusion factor
    Lo_Diffuse *= ao;

    // Final scene color, depending on visual mode (0 = full rendering, 2 = radiance visualization)
    if(uSettings.visualMode != 2) {
        SceneColor = Lo_Diffuse + Lo_Specular + emissive;  // Regular rendering (diffuse + specular + emissive)
    } else {
        SceneColor = radiance;  // Show only indirect lighting (radiance)
    }

    // If the scene color exceeds 1.0, mark it as bright for bloom processing
    if (SceneColor.r > 1.0 || SceneColor.g > 1.0 || SceneColor.b > 1.0) {
        SceneColorBright = SceneColor;
    } else {
        SceneColorBright = vec3(0);
    }
}
