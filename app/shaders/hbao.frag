#version 460 core

#include "lib/depth.glsl"
#include "lib/math.glsl"

layout(location = 0) in vec2 vTexCoords;

layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform Camera {
    vec3 position;
    mat4 view;
    mat4 projection;
    mat4 inverseView;
    mat4 inverseProjection;
} uCamera;

layout(binding = 0) uniform sampler2D gDepth;
layout(binding = 1) uniform sampler2D gNormal;
layout(binding = 2) uniform sampler2D NoiseMap;

uniform float uHBAO_radius;
uniform float uHBAO_bias;
uniform float uHBAO_negInvRadius;
uniform int uHBAO_maxRadiusPixels;
uniform int uHBAO_stepCount;
uniform int uHBAO_directionCount;

float falloff(float distanceSquare) {
    return distanceSquare * uHBAO_negInvRadius + 1.0;
}

float computeAO(vec3 p, vec3 n, vec3 s) {
    vec3 v = s - p;
    float VdotV = dot(v, v);
    float NdotV = dot(n, v) * inversesqrt(VdotV);

    return clamp(NdotV - uHBAO_bias, 0.0, 1.0) * clamp(falloff(VdotV), 0.0, 1.0);
}

void main() {
    const float depth = texture(gDepth, vTexCoords).r;
    if(depth >= 1.0)
        discard;

    const vec3 fragPosViewSpace = viewPositionFromDepth(depth, vTexCoords, uCamera.inverseProjection);

    const vec2 gBufferSize = textureSize(gDepth, 0);
    const vec2 noiseSize = textureSize(NoiseMap, 0);
    const vec2 noiseTexCoord = (gBufferSize / noiseSize) * vTexCoords;
    const vec2 rand = texture(NoiseMap, noiseTexCoord).xy;

    const vec4 screenSize = vec4(gBufferSize.x, gBufferSize.y, 1.0 / gBufferSize.x, 1.0 / gBufferSize.y);

    vec3 N = normalize(texture(gNormal, vTexCoords).rgb);
    N = mat3(uCamera.view) * N;

    float stepSize = min(uHBAO_radius / fragPosViewSpace.z, uHBAO_maxRadiusPixels) / (uHBAO_stepCount + 1.0);
    float stepAngle = TWO_PI / uHBAO_directionCount;

    float ao = 0;

    for(int d = 0; d < uHBAO_directionCount; ++d) {
        float angle = stepAngle * (float(d) + rand.x);

        float cosAngle = cos(angle);
        float sinAngle = sin(angle);

        vec2 direction = vec2(cosAngle, sinAngle);

        float rayPixels = fract(rand.y) * stepSize + 1.0;

        for(int s = 0; s < uHBAO_stepCount; ++s) {
            const vec2 snappedUV = round(rayPixels * direction) * screenSize.zw + vTexCoords;
            const float tempDepth = texture(gDepth, snappedUV).r;
            const vec3 tempFragPosViewSpace = viewPositionFromDepth(tempDepth, snappedUV, uCamera.inverseProjection);
            rayPixels += stepSize;
            float tempAO = computeAO(fragPosViewSpace, N, tempFragPosViewSpace);
            ao += tempAO;
        }
    }

    FragColor = vec4(1 - ao);
}