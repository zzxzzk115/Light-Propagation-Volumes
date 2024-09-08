#ifndef DEPTH_GLSL
#define DEPTH_GLSL

// Convert a clip-space position to view-space using the inverse projection matrix
vec3 clipToView(vec4 v, mat4 inverseProjection) {
    const vec4 view = inverseProjection * v; // Transform clip space to view space
    return view.xyz / view.w; // Divide by w to convert from homogeneous coordinates
}

// Retrieve the depth value from a depth texture, remapped from [0, 1] to [-1, 1]
float getDepth(sampler2D depthMap, vec2 texCoord) {
    const float sampledDepth = texture(depthMap, texCoord).r; // Sample depth texture
    return sampledDepth * 2.0 - 1.0; // Remap depth to [-1, 1] range
}

// Reconstruct view-space position from depth and texture coordinates
vec3 viewPositionFromDepth(float z, vec2 texCoord, mat4 inverseProjection) {
    // Transform texture coordinates and depth to clip space and convert to view space
    return clipToView(vec4(texCoord * 2.0 - 1.0, z, 1.0), inverseProjection);
}

#endif
