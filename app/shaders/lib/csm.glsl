#ifndef CSM_GLSL
#define CSM_GLSL

#include "../common/csm_uniform.glsl"

// Calculate light visibility using shadow mapping with Percentage-Closer Filtering (PCF)
float getLightVisibility(uint cascadeIndex, vec3 fragPos, sampler2DArrayShadow cascadedShadowMaps) {
    // Transform fragment position to light space for the given cascade
    vec4 shadowCoord = uCascades.lightSpaceMatrices[cascadeIndex] * vec4(fragPos, 1.0);

    const float bias = 0.005; // Bias to avoid shadow acne

    // Uncomment below to use simple shadow lookup without PCF
    // return texture(cascadedShadowMaps, vec4(shadowCoord.xy, cascadeIndex, shadowCoord.z - bias));

    // PCF (Percentage-Closer Filtering) setup
    const ivec2 shadowMapSize = textureSize(cascadedShadowMaps, 0).xy; // Get shadow map size
    const float kScale = 1.0; // Scaling factor for shadow sampling
    const float dx = kScale * 1.0 / float(shadowMapSize.x); // X offset for sampling
    const float dy = kScale * 1.0 / float(shadowMapSize.y); // Y offset for sampling

    const int kRange = 1; // Sampling range for PCF
    float shadowFactor = 0.0; // Accumulate shadow results
    uint count = 0; // Track number of samples

    // Perform PCF by sampling neighboring texels in a grid pattern
    for(int x = -kRange; x <= kRange; ++x) {
        for(int y = -kRange; y <= kRange; ++y) {
            shadowFactor += texture(cascadedShadowMaps, vec4(shadowCoord.xy + vec2(dx * x, dy * y), cascadeIndex, shadowCoord.z - bias));
            count++;
        }
    }

    // Average the shadow results from all samples
    return shadowFactor / float(count);
}

#endif
