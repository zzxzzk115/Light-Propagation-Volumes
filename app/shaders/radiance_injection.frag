#version 460 core

#include "lib/math.glsl"
#include "lib/lpv.glsl"

// Input data from the fragment shader
layout(location = 0) in FragData {
    vec3 normal;
    vec3 flux;
} fs_in;

// Output spherical harmonics coefficients
layout(location = 0) out vec4 SH_R;
layout(location = 1) out vec4 SH_G;
layout(location = 2) out vec4 SH_B;

void main() {
    // Discard if normal is too small
    if (length(fs_in.normal) < 0.01) {
        discard;
    }

    // Evaluate spherical harmonics coefficients for cosine lobe
    const vec4 SH_Coefficients = SH_EvaluateCosineLobe(fs_in.normal) / PI;

    // Calculate and store the spherical harmonics for each color channel
    SH_R = SH_Coefficients * fs_in.flux.r;
    SH_G = SH_Coefficients * fs_in.flux.g;
    SH_B = SH_Coefficients * fs_in.flux.b;
}
