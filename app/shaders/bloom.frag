#version 460 core

layout(location = 0) in vec2 v_TexCoords;

layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform sampler2D sceneColor;
layout(binding = 1) uniform sampler2D bloomBlur;

uniform float bloomFactor;

void main() {
    vec3 hdrColor = texture(sceneColor, v_TexCoords).rgb;
    vec3 bloomColor = texture(bloomBlur, v_TexCoords).rgb;
    hdrColor += bloomColor * bloomFactor;

    FragColor = vec4(hdrColor, 1);
}
