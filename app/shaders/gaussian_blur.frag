#version 460 core

// https://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/

// Constants for Gaussian blur
#define getTexelSize(src) (1.0 / textureSize(src, 0))

const float kOffsets[3] = { 0.0, 1.3846153846, 3.2307692308 };
const float kWeights[3] = { 0.2270270270, 0.3162162162, 0.0702702703 };

// Input texture coordinates
layout(location = 0) in vec2 varingTexCoords;

// Output fragment color
layout(location = 0) out vec3 FragColor;

// Input texture and parameters
layout(location = 0, binding = 0) uniform sampler2D texture0;
layout(location = 1) uniform bool horizontal; // Blur direction
layout(location = 2) uniform float scale = 1.0; // Blur scale

// Horizontal blur function
vec3 HorizontalBlur() {
    const float texOffset = getTexelSize(texture0).x * scale;
    vec3 result = texture(texture0, varingTexCoords).rgb * kWeights[0];
    for(uint i = 1; i < 3; ++i) {
        result += texture(texture0, varingTexCoords + vec2(texOffset * i, 0.0)).rgb * kWeights[i];
        result += texture(texture0, varingTexCoords - vec2(texOffset * i, 0.0)).rgb * kWeights[i];
    }
    return result;
}

// Vertical blur function
vec3 VerticalBlur() {
    const float texOffset = getTexelSize(texture0).y * scale;
    vec3 result = texture(texture0, varingTexCoords).rgb * kWeights[0];
    for(uint i = 1; i < 3; ++i) {
        result += texture(texture0, varingTexCoords + vec2(0.0, texOffset * i)).rgb * kWeights[i];
        result += texture(texture0, varingTexCoords - vec2(0.0, texOffset * i)).rgb * kWeights[i];
    }
    return result;
}

void main() {
    // Apply horizontal or vertical blur based on the uniform
    FragColor = horizontal ? HorizontalBlur() : VerticalBlur();
}
