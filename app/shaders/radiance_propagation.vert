#version 460 core

#include "lib/lpv.glsl"

// Input uniform data for radiance injection
layout(location = 0) uniform RadianceInjection uInjection;

// Output vertex data
layout(location = 0) out VertexData {
    flat ivec3 cellIndex;
} vs_out;

void main() {
    // Compute the grid size and position based on the vertex ID
    uvec2 gridSize = uvec2(uInjection.gridSize.xy);
    const vec3 position = vec3(gl_VertexID % gridSize.x, 
                                (gl_VertexID / gridSize.x) % gridSize.y, 
                                gl_VertexID / (gridSize.x * gridSize.y));

    // Assign the cell index
    vs_out.cellIndex = ivec3(position);

    // Convert position to normalized device coordinates (NDC)
    const vec2 ndc = (position.xy + 0.5) / uInjection.gridSize.xy * 2.0 - 1.0;

    // Set the final position of the vertex
    gl_Position = vec4(ndc, 0.0, 1.0);
    gl_PointSize = 1.0;
}
