#version 460 core

layout(location = 0) in vec2 v_TexCoords;

layout(binding = 0) uniform sampler2D target;

layout(location = 0) out vec3 FragColor;

void main() {
    FragColor = texture(target, v_TexCoords).rgb;
}