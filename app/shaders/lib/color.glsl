#ifndef COLOR_GLSL
#define COLOR_GLSL

// Convert color from linear space to gamma space (gamma correction)
vec3 linearToGamma(vec3 color) {
    return pow(color, vec3(1.0 / 2.2)); // Apply gamma correction with gamma 2.2
}

// Convert color from gamma space to linear space (inverse gamma correction)
vec3 gammaToLinear(vec3 color) {
    return pow(color, vec3(2.2)); // Apply inverse gamma correction
}

// Apply Reinhard tone mapping to color (used for high dynamic range rendering)
vec3 toneMapReinhard(vec3 color) {
    return color / (color + vec3(1.0)); // Simple tone mapping that prevents values from exceeding 1
}

// Apply ACES tone mapping (more advanced tone mapping for cinematic look)
vec3 toneMapACES(vec3 color) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    // Apply the ACES tone mapping formula and clamp the result between 0 and 1
    return clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);
}

#endif
