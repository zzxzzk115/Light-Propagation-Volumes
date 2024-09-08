#version 460 core

#include "lib/pbr.glsl"

// Input attributes
layout(location = 0) in vec2 vTexCoords;    // Texture coordinates
layout(location = 1) in vec3 vFragPos;      // Fragment position
layout(location = 2) in mat3 vTBN;          // Tangent-Bitangent-Normal matrix

// Output attributes
layout(location = 0) out vec3 RSMPosition;  // Output position for RSM
layout(location = 1) out vec3 RSMNormal;    // Output normal for RSM
layout(location = 2) out vec3 RSMFlux;      // Output flux for RSM

// Uniforms
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

layout(binding = 0) uniform sampler2D uTextures[5]; // Array of texture samplers

void main() {
    // Base color and alpha
    vec3 baseColor = vec3(0);
    float alpha = 1.0;
    if (uMaterial.baseColorTextureIndex != -1) {
        vec4 color = texture(uTextures[uMaterial.baseColorTextureIndex], vTexCoords);
        baseColor = color.rgb;
        alpha = color.a;
    }

    // Discard if alpha is below threshold
    if (alpha < 0.5) {
        discard;
    }

    // Metallic and roughness
    float metallic = 0.0;
    float roughness = 0.5;
    if (uMaterial.metallicRoughnessTextureIndex != -1) {
        vec4 metallicRoughness = texture(uTextures[uMaterial.metallicRoughnessTextureIndex], vTexCoords);
        metallic = metallicRoughness.b;
        roughness = metallicRoughness.g;
    }

    // Normal calculation
    vec3 normal = normalize(vTBN[2]);
    if (uMaterial.normalTextureIndex != -1) {
        vec3 normalColor = texture(uTextures[uMaterial.normalTextureIndex], vTexCoords).rgb;
        vec3 tangentNormal = normalColor * 2.0 - 1.0;
        normal = tangentNormal * transpose(vTBN);
    }

    // Ambient occlusion
    float ao = 1.0;
    if (uMaterial.occlusionTextureIndex != -1) {
        ao = texture(uTextures[uMaterial.occlusionTextureIndex], vTexCoords).r;
    }

    // Emissive color
    vec3 emissive = vec3(0);
    if (uMaterial.emissiveTextureIndex != -1) {
        emissive = texture(uTextures[uMaterial.emissiveTextureIndex], vTexCoords).rgb;
    }

    // RSM outputs (position, normal and flux)
    RSMPosition = vFragPos;
    RSMNormal = normal;
    RSMFlux = baseColor * uLight.intensity + emissive;
}
