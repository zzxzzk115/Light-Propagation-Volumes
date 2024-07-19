#ifndef DEPTH_GLSL
#define DEPTH_GLSL

vec3 clipToView(vec4 v, mat4 inverseProjection) {
    const vec4 view = inverseProjection * v;
    return view.xyz / view.w;
}

float getDepth(sampler2D depthMap, vec2 texCoord) {
    const float sampledDepth = texture(depthMap, texCoord).r;
    return sampledDepth * 2.0 - 1.0;
}

vec3 viewPositionFromDepth(float z, vec2 texCoord, mat4 inverseProjection) {
    return clipToView(vec4(texCoord * 2.0 - 1.0, z, 1.0), inverseProjection);
}

#endif