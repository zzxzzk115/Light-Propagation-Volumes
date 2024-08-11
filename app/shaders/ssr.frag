#version 460 core

#include "lib/depth.glsl"
#include "lib/math.glsl"

#define EPSILON 1e-4
#define MAX_RAY_DISTANCE 200

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
layout(binding = 2) uniform sampler2D gMetallicRoughnessAO;
layout(binding = 3) uniform sampler2D sceneColor;

uniform float reflectionFactor;

vec3 F_Schlick(vec3 F0, vec3 albedo, float NdotV, float roughness) {
    float roughnessFactor = pow(1.0 - roughness, 5.0);
    float fresnelFactor = pow(1.0 - NdotV, 5.0);

    vec3 fresnel = F0 + (albedo - F0) * fresnelFactor;

    return fresnel * roughnessFactor + albedo * (1.0 - roughnessFactor);
}

bool isRayOutOfScreen(vec2 ray) {
    return (ray.x > 1.0 || ray.y > 1.0 || ray.x < 0.0 || ray.y < 0.0);
}

// Simple hash function for jittering
vec3 hash(vec3 a) {
    const vec3 scale = vec3(0.8);
    const float K = 19.19;

    a = fract(a * scale);
    a += dot(a, a.yxz + K);
    return fract((a.xxy + a.yxx) * a.zyx);
}

// Ray marching with dynamic step size
vec3 rayMarch(vec3 rayPos, vec3 dir, int iterationCount, float roughness) {
    float sampleDepth;
    vec3 hitColor = vec3(0);
    bool hit = false;

    float stepSize = mix(1.0, 0.25, roughness);  // Step size varies with roughness

    for (int i = 0; i < iterationCount; ++i) {
        rayPos += dir * stepSize;

        if (isRayOutOfScreen(rayPos.xy)) {
            break;
        }

        sampleDepth = texture(gDepth, rayPos.xy).r;
        float depthDiff = rayPos.z - sampleDepth;

        if (depthDiff >= -EPSILON && depthDiff < EPSILON) {
            hit = true;
            hitColor = textureLod(sceneColor, rayPos.xy, roughness * 5.0).rgb;  // Use Mipmap based on roughness
            break;
        }
    }

    return hitColor;
}

void main() {
    vec2 screenSize = textureSize(gDepth, 0);

    float depth = texture(gDepth, vTexCoords).r;
    if (depth >= 1.0) {
        discard;
        return;
    }

    vec3 fragPosTextureSpace = vec3(vTexCoords, depth);

    vec3 N = normalize(texture(gNormal, vTexCoords).rgb);
    N = mat3(uCamera.view) * N;

    vec3 fragPosViewSpace = viewPositionFromDepth(depth, vTexCoords, uCamera.inverseProjection);

    vec3 V = normalize(-fragPosViewSpace);  // View direction
    vec3 R = normalize(reflect(-V, N));     // Reflected direction

    // Roughness and metallic retrieval
    vec3 gBufferValues = texture(gMetallicRoughnessAO, vTexCoords).rgb;
    float roughness = clamp(gBufferValues.g, 0.0, 1.0);
    float metallic = clamp(gBufferValues.r, 0.0, 1.0);

    // Early exit for non-reflective or rough surfaces
    if (roughness > 0.9 || metallic < 0.1) {
        discard;
        return;
    }

    // Add jitter to reduce banding
    vec3 jitter = hash(fragPosViewSpace) * roughness * 0.1;
    jitter.x = 0.0;
    R = normalize(R + jitter);

    if (R.z > 0.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec3 rayEndPosViewSpace = fragPosViewSpace + R * MAX_RAY_DISTANCE;
    vec4 rayEndPosClipSpace = uCamera.projection * vec4(rayEndPosViewSpace, 1.0);
    rayEndPosClipSpace /= rayEndPosClipSpace.w;  // Perspective divide to get clip space coordinates

    vec3 rayEndPosTextureSpace = rayEndPosClipSpace.xyz * 0.5 + 0.5;

    vec3 rayDirTextureSpace = normalize(rayEndPosTextureSpace - fragPosTextureSpace);

    // Calculate max distance in screen space
    int maxDistanceScreenSpace = int(MAX_RAY_DISTANCE / length(R.xy));

    // Perform ray marching with optimizations
    vec3 color = rayMarch(fragPosTextureSpace, rayDirTextureSpace / maxDistanceScreenSpace, maxDistanceScreenSpace, roughness);

    // Fresnel and visibility factor calculation
    vec3 albedo = texture(sceneColor, vTexCoords).rgb;
    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    float NdotV = clamp(dot(N, V), 0.0, 1.0);
    vec3 F = F_Schlick(F0, vec3(1.0), NdotV, roughness);
    float visibility = 1.0 - max(dot(V, R), 0.0);

    // Screen edge factor
    vec2 dCoords = smoothstep(0.2, 0.6, abs(vec2(0.5) - rayEndPosTextureSpace.xy));
    float screenEdgeFactor = clamp(1.0 - (dCoords.x + dCoords.y), 0.0, 1.0);

    float reflectionMultiplier = reflectionFactor * pow(metallic, 3.0) * screenEdgeFactor * -R.z;

    // Final color composition
    FragColor = vec4(color * F * visibility * clamp(reflectionMultiplier, 0.0, 1.0), 1.0);
}
