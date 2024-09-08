#version 460 core

// Cell sides and neighbor orientations
const vec2 kCellSides[4] = {
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(-1.0, 0.0),
    vec2(0.0, -1.0),
};

const mat3 kNeighbourOrientations[6] = {
    mat3(1, 0, 0, 0, 1, 0, 0, 0, 1),   // Z+
    mat3(-1, 0, 0, 0, 1, 0, 0, 0, -1), // Z-
    mat3(0, 0, 1, 0, 1, 0, -1, 0, 0),  // X+
    mat3(0, 0, -1, 0, 1, 0, 1, 0, 0),  // X-
    mat3(1, 0, 0, 0, 0, 1, 0, -1, 0),  // Y+
    mat3(1, 0, 0, 0, 0, -1, 0, 1, 0)   // Y-
};

#include "lib/math.glsl"
#include "lib/lpv.glsl"

// Input data
layout(location = 0) in FragData {
    flat ivec3 cellIndex;
} fs_in;

// Textures for spherical harmonics
layout(binding = 0) uniform sampler3D SH_R;
layout(binding = 1) uniform sampler3D SH_G;
layout(binding = 2) uniform sampler3D SH_B;

// Output spherical harmonics coefficients
layout(location = 0) out vec4 Propagated_SH_R;
layout(location = 1) out vec4 Propagated_SH_G;
layout(location = 2) out vec4 Propagated_SH_B;

// Get side direction for evaluation
vec3 getEvalSideDirection(int index, mat3 orientation) {
    const vec2 side = kCellSides[index];
    return orientation * vec3(side.x * 0.4472135, side.y * 0.4472135, 0.894427);
}

// Get side direction for reprojection
vec3 getReprojSideDirection(int index, mat3 orientation) {
    const vec2 side = kCellSides[index];
    return orientation * vec3(side.x, side.y, 0);
}

// Compute SH contributions from neighboring cells
SH_Coefficients getContributions(ivec3 cellIndex) {
    SH_Coefficients contribution = {vec4(0.0), vec4(0.0), vec4(0.0)};
    for(int neighbour = 0; neighbour < 6; ++neighbour) {
        const mat3 orientation = kNeighbourOrientations[neighbour];
        const vec3 mainDirection = orientation * vec3(0.0, 0.0, 1.0);

        const ivec3 neighbourIndex = cellIndex - ivec3(mainDirection);
        
        const SH_Coefficients neighbourCoeffs = {
            texelFetch(SH_R, neighbourIndex, 0),
            texelFetch(SH_G, neighbourIndex, 0),
            texelFetch(SH_B, neighbourIndex, 0),
        };

        const float kSolidAngle = 0.4006696846 / PI;

        const vec4 mainDirectionCosineLobeSH = SH_EvaluateCosineLobe(mainDirection);
        const vec4 mainDirectionSH = SH_Evaluate(mainDirection);
        contribution.red += kSolidAngle * dot(neighbourCoeffs.red, mainDirectionSH) * mainDirectionCosineLobeSH;
        contribution.green += kSolidAngle * dot(neighbourCoeffs.green, mainDirectionSH) * mainDirectionCosineLobeSH;
        contribution.blue += kSolidAngle * dot(neighbourCoeffs.blue, mainDirectionSH) * mainDirectionCosineLobeSH;

        const float kSideFaceSubtendedSolidAngle = 0.4234413544 / PI;

        for(int sideIndex = 0; sideIndex < 4; ++sideIndex) {
            const vec3 evalSideDirection = getEvalSideDirection(sideIndex, orientation);
            const vec3 reprojSideDirection = getReprojSideDirection(sideIndex, orientation);

            const vec4 reprojSideDirectionCosineLobeSH = SH_EvaluateCosineLobe(reprojSideDirection);
            const vec4 evalSideDirectionSH = SH_Evaluate(evalSideDirection);

            contribution.red += kSideFaceSubtendedSolidAngle * dot(neighbourCoeffs.red, evalSideDirectionSH) * reprojSideDirectionCosineLobeSH;
            contribution.green += kSideFaceSubtendedSolidAngle * dot(neighbourCoeffs.green, evalSideDirectionSH) * reprojSideDirectionCosineLobeSH;
            contribution.blue += kSideFaceSubtendedSolidAngle * dot(neighbourCoeffs.blue, evalSideDirectionSH) * reprojSideDirectionCosineLobeSH;
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
