#version 460 core

#include "lib/math.glsl"
#include "lib/lpv.glsl"

layout(location = 0) in FragData {
    vec3 normal;
    vec3 flux;
} fs_in;

layout(location = 0) out vec4 SH_R;
layout(location = 1) out vec4 SH_G;
layout(location = 2) out vec4 SH_B;

void main() {
    if(length(fs_in.normal) < 0.01) {
        discard;
    }

    const vec4 SH_Coefficients = SH_EvaluateCosineLobe(fs_in.normal) / PI;

    SH_R = SH_Coefficients * fs_in.flux.r;
    SH_G = SH_Coefficients * fs_in.flux.g;
    SH_B = SH_Coefficients * fs_in.flux.b;
}