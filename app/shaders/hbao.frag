#version 460 core

// references:
// https://developer.download.nvidia.cn/presentations/2008/SIGGRAPH/HBAO_SIG08b.pdf
// https://citeseerx.ist.psu.edu/document?repid=rep1&type=pdf&doi=13bc73f19c136873cda61696aee8e90e2ce0f2d8

#include "lib/depth.glsl"
#include "lib/math.glsl"

layout(location = 0) in vec2 vTexCoords;
layout(location = 0) out float FragColor;

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
uniform float uHBAO_intensity;
uniform float uHBAO_negInvRadius2;
uniform int uHBAO_maxRadiusPixels;
uniform int uHBAO_stepCount;
uniform int uHBAO_directionCount;

float falloff(float distanceSquare) {
    return distanceSquare * uHBAO_negInvRadius2 + 1.0;
}

float computeAO(vec3 p, vec3 n, vec3 s, inout float top) {
    vec3 h = s - p;
    float dist = sqrt(dot(h, h));
    float sinBlock = dot(n, h) / dist;
    float diff = max(sinBlock - top - uHBAO_bias, 0);
    top = max(sinBlock, top);
    float attenuation = 1.0 / (1.0 + dist * dist);
    return clamp(diff, 0.0, 1.0) * clamp(falloff(dist), 0.0, 1.0) * attenuation;
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

    // vec3 N = normalize(texture(gNormal, vTexCoords).rgb);
    // N = mat3(uCamera.view) * N;
    vec3 dpdx = dFdx(fragPosViewSpace);
    vec3 dpdy = dFdy(fragPosViewSpace);
    vec3 N = normalize(cross(dpdx, dpdy));

    float stepSize = min(uHBAO_radius / fragPosViewSpace.z, float(uHBAO_maxRadiusPixels)) / float(uHBAO_stepCount + 1);
    float stepAngle = TWO_PI / float(uHBAO_directionCount);

    float ao = 0.0;

    for(int d = 0; d < uHBAO_directionCount; ++d) {
        float angle = stepAngle * (float(d) + rand.x);

        float cosAngle = cos(angle);
        float sinAngle = sin(angle);

        vec2 direction = vec2(cosAngle, sinAngle);

        float rayPixels = fract(rand.y) * stepSize;
        float top = 0;

        for(int s = 0; s < uHBAO_stepCount; ++s) {
            const vec2 sampleUV = vTexCoords + direction * rayPixels * screenSize.zw;
            const float tempDepth = texture(gDepth, sampleUV).r;
            const vec3 tempFragPosViewSpace = viewPositionFromDepth(tempDepth, sampleUV, uCamera.inverseProjection);
            rayPixels += stepSize;
            float tempAO = computeAO(fragPosViewSpace, N, tempFragPosViewSpace, top);
            ao += tempAO;
        }
    }

    FragColor = 1.0 - ao * uHBAO_intensity / float(uHBAO_directionCount * uHBAO_stepCount);
}
