#version 460 core

layout(location = 0) out vec2 vTexCoords;

// glDrawArrays(GL_TRIANGLES, 3);
void main() {
    vTexCoords = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(vTexCoords * 2.0 - 1.0, 0.0, 1.0);
}