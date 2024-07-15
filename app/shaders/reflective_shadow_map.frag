#version 450

#include "lib/pbr.glsl"

layout(location = 0) in vec2 vTexCoords;
layout(location = 1) in vec3 vFragPos;
layout(location = 2) in mat3 vTBN;

layout(location = 0) out vec3 RSMPosition;
layout(location = 1) out vec3 RSMNormal;
layout(location = 2) out vec3 RSMFlux;

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

layout(binding = 0) uniform sampler2D pbrTextures[5];

void main() {
    vec3 baseColor = vec3(0);
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

    vec3 emissive = vec3(0);
    if(uMaterial.emissiveTextureIndex != -1) {
        emissive = texture(pbrTextures[uMaterial.emissiveTextureIndex], vTexCoords).rgb;
    }

    RSMPosition = vFragPos;
    RSMNormal = normal;
    RSMFlux = baseColor * uLight.intensity + emissive;
}