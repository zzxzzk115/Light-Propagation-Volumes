#version 460 core

#include "lib/lpv.glsl"

// Input textures
layout(binding = 0) uniform sampler2D RSMPosition;
layout(binding = 1) uniform sampler2D RSMNormal;
layout(binding = 2) uniform sampler2D RSMFlux;

// Uniform for grid parameters
layout(location = 0) uniform RadianceInjection uInjection;

// Output data
layout(location = 0) out VertexData {
    vec3 normal;
    vec3 flux;
    flat ivec3 cellIndex;
} vs_out;

void main() {
    // Compute texture coordinates from vertex ID
    const ivec2 uv = ivec2(gl_VertexID % uInjection.rsmResolution, gl_VertexID / uInjection.rsmResolution);

    // Fetch position, normal, and flux from RSM textures
    const vec3 position = texelFetch(RSMPosition, uv, 0).xyz;
    const vec3 normal = texelFetch(RSMNormal, uv, 0).xyz;
    const vec3 flux = texelFetch(RSMFlux, uv, 0).xyz;

    // Set output variables
    vs_out.normal = normal;
    vs_out.flux = flux;

    // Calculate the cell index for the grid
    const ivec3 cellIndex = ivec3((position - uInjection.gridAABBMin) / uInjection.gridCellSize + 0.5 * normal);
    vs_out.cellIndex = cellIndex;

    // Convert cell index to NDC space for rendering
    const vec2 ndc = (vec2(cellIndex.xy) + 0.5) / uInjection.gridSize.xy * 2.0 - 1.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
    gl_PointSize = 1.0;
}
