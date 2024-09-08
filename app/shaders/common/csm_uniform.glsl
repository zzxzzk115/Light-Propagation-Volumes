#ifndef CSM_UNIFORM_GLSL
#define CSM_UNIFORM_GLSL

// Define the maximum number of shadow cascades
#define MAX_NUM_CASCADES 1

// Uniform block for cascade shadow mapping
// 'splitDepth' holds the depth values for splitting between cascades
// 'lightSpaceMatrices' stores the light space transformation matrices for each cascade
layout(binding = 2) uniform Cascades {
    vec4 splitDepth; // Depth thresholds for splitting cascades
    mat4 lightSpaceMatrices[MAX_NUM_CASCADES]; // Light space matrices for each cascade
} uCascades;

// Function to select the appropriate cascade based on fragment position in view space
uint selectCascadeIndex(vec3 fragPosViewSpace) {
    uint cascadeIndex = 0;
    // Loop through cascades to find the one corresponding to the fragment's depth
    for(uint i = 0; i < MAX_NUM_CASCADES - 1; ++i) {
        // Check if the fragment is in the current cascade
        if(fragPosViewSpace.z < uCascades.splitDepth[i]) {
            cascadeIndex = i + 1;
        }
    }
    return cascadeIndex;
}

#endif
