#ifndef CSM_GLSL
#define CSM_GLSL

#include "../common/csm_uniform.glsl"

float getLightVisibility(uint cascadeIndex, vec3 fragPos, sampler2DArrayShadow cascadedShadowMaps) {
    vec4 shadowCoord = uCascades.lightSpaceMatrices[cascadeIndex] * vec4(fragPos, 1.0);

    const float bias = 0.005;

    // return texture(cascadedShadowMaps, vec4(shadowCoord.xy, cascadeIndex, shadowCoord.z - bias));

    // PCF
    const ivec2 shadowMapSize = textureSize(cascadedShadowMaps, 0).xy;
    const float kScale = 1.0;
    const float dx = kScale * 1.0 / float(shadowMapSize.x);
    const float dy = kScale * 1.0 / float(shadowMapSize.y);

    const int kRange = 1;
    float shadowFactor = 0.0;
    uint count = 0;
    for(int x = -kRange; x <= kRange; ++x) {
        for(int y = -kRange; y <= kRange; ++y) {
            shadowFactor += texture(cascadedShadowMaps, vec4(shadowCoord.xy + vec2(dx * x, dy * y), cascadeIndex, shadowCoord.z - bias));
            count++;
        }
    }
    return shadowFactor / float(count);
}

#endif