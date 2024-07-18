#version 460 core

#include "lib/lpv.glsl"

layout(binding = 0) uniform sampler2D RSMPosition;
layout(binding = 1) uniform sampler2D RSMNormal;
layout(binding = 2) uniform sampler2D RSMFlux;

layout(location = 0) uniform RadianceInjection uInjection;

layout(location = 0) out VertexData {
    vec3 normal;
    vec3 flux;
    flat ivec3 cellIndex;
} vs_out;

void main() {
    const ivec2 uv = ivec2(gl_VertexID % uInjection.rsmResolution, gl_VertexID / uInjection.rsmResolution);

    const vec3 position = texelFetch(RSMPosition, uv, 0).xyz;
    const vec3 normal = texelFetch(RSMNormal, uv, 0).xyz;
    const vec3 flux = texelFetch(RSMFlux, uv, 0).xyz;

    vs_out.normal = normal;
    vs_out.flux = flux;

    const ivec3 cellIndex = ivec3((position - uInjection.gridAABBMin) / uInjection.gridCellSize + 0.5 * normal);
    vs_out.cellIndex = cellIndex;

    const vec2 ndc = (vec2(cellIndex.xy) + 0.5) / uInjection.gridSize.xy * 2.0 - 1.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
    gl_PointSize = 1.0;
}