#version 460 core

#include "lib/math.glsl"
#include "lib/lpv.glsl"

layout(location = 0) in FragData {
    flat ivec3 cellIndex;
} fs_in;

layout(binding = 0) uniform sampler3D SH_R;
layout(binding = 1) uniform sampler3D SH_G;
layout(binding = 2) uniform sampler3D SH_B;

layout(location = 0) out vec4 Propagated_SH_R;
layout(location = 1) out vec4 Propagated_SH_G;
layout(location = 2) out vec4 Propagated_SH_B;

// clang-format off
const vec2 kCellSides[4] = {
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(-1.0, 0.0),
    vec2(0.0, -1.0),
};
// clang-format on

// orientation = [ right | up | forward ] = [ x | y | z ]
vec3 getEvalSideDirection(int index, mat3 orientation) {
    const float smallComponent = 0.4472135; // 1 / sqrt(5)
    const float bigComponent = 0.894427;    // 2 / sqrt(5)

    const vec2 side = kCellSides[index];

    return orientation * vec3(side.x * smallComponent, side.y * smallComponent, bigComponent);
}

vec3 getReprojSideDirection(int index, mat3 orientation) {
    const vec2 side = kCellSides[index];
    return orientation * vec3(side.x, side.y, 0);
}

// clang-format off
const mat3 kNeighbourOrientations[6] = {
    mat3(1, 0, 0, 0, 1, 0, 0, 0, 1),   // Z+
    mat3(-1, 0, 0, 0, 1, 0, 0, 0, -1), // Z-
    mat3(0, 0, 1, 0, 1, 0, -1, 0, 0),  // X+
    mat3(0, 0, -1, 0, 1, 0, 1, 0, 0),  // X-
    mat3(1, 0, 0, 0, 0, 1, 0, -1, 0),  // Y+
    mat3(1, 0, 0, 0, 0, -1, 0, 1, 0)   // Y-
};
// clang-format on

SH_Coefficients getContributions(ivec3 cellIndex) {
    SH_Coefficients contribution = {vec4(0.0), vec4(0.0), vec4(0.0)};
    for(int neighbour = 0; neighbour < 6; ++neighbour) {
        const mat3 orientation = kNeighbourOrientations[neighbour];
        const vec3 mainDirection = orientation * vec3(0.0, 0.0, 1.0);

        const ivec3 neighbourIndex = cellIndex - ivec3(mainDirection);
        
        // clang-format off
        const SH_Coefficients neighbourCoeffs = {
            texelFetch(SH_R, neighbourIndex, 0),
            texelFetch(SH_G, neighbourIndex, 0),
            texelFetch(SH_B, neighbourIndex, 0),
        };
        // clang-format on

        const float kDirectFaceSubtendedSolidAngle = 0.4006696846 / PI;

        const vec4 mainDirectionCosineLobeSH = SH_EvaluateCosineLobe(mainDirection);
        const vec4 mainDirectionSH = SH_Evaluate(mainDirection);
        contribution.red += kDirectFaceSubtendedSolidAngle * dot(neighbourCoeffs.red, mainDirectionSH) * mainDirectionCosineLobeSH;
        contribution.green += kDirectFaceSubtendedSolidAngle * dot(neighbourCoeffs.green, mainDirectionSH) * mainDirectionCosineLobeSH;
        contribution.blue += kDirectFaceSubtendedSolidAngle * dot(neighbourCoeffs.blue, mainDirectionSH) * mainDirectionCosineLobeSH;

        const float kSideFaceSubtendedSolidAngle = 0.4234413544 / PI;

        for(int sideFace = 0; sideFace < 4; ++sideFace) {
            const vec3 evalDirection = getEvalSideDirection(sideFace, orientation);
            const vec3 reprojDirection = getReprojSideDirection(sideFace, orientation);

            const vec4 reprojDirectionCosineLobeSH = SH_EvaluateCosineLobe(reprojDirection);
            const vec4 evalDirectionSH = SH_Evaluate(evalDirection);

            contribution.red += kSideFaceSubtendedSolidAngle * dot(neighbourCoeffs.red, evalDirectionSH) * reprojDirectionCosineLobeSH;
            contribution.green += kSideFaceSubtendedSolidAngle * dot(neighbourCoeffs.green, evalDirectionSH) * reprojDirectionCosineLobeSH;
            contribution.blue += kSideFaceSubtendedSolidAngle * dot(neighbourCoeffs.blue, evalDirectionSH) * reprojDirectionCosineLobeSH;
        }
    }

    return contribution;
}

void main() {
    const SH_Coefficients c = getContributions(fs_in.cellIndex);

    Propagated_SH_R = c.red;
    Propagated_SH_G = c.green;
    Propagated_SH_B = c.blue;
}