#ifndef LPV_GLSL
#define LPV_GLSL

// Structure to store parameters for Radiance Injection into the Light Propagation Volume (LPV)
struct RadianceInjection {
    vec3 gridAABBMin; // Minimum corner of the grid's AABB (Axis-Aligned Bounding Box)
    vec3 gridSize;    // Size of the LPV grid (in cells)
    float gridCellSize; // Size of each cell in the grid
    int rsmResolution;  // Resolution of the Reflective Shadow Map (RSM)
};

// Spherical Harmonics (SH) constants for evaluation
#define SH_C0 0.282094791 // SH constant for the zeroth coefficient (1 / 2sqrt(pi))
#define SH_C1 0.488602512 // SH constant for the first-order coefficients (sqrt(3/pi) / 2)

// Evaluate the first 4 Spherical Harmonics basis functions given a direction
vec4 SH_Evaluate(vec3 direction) {
    direction = normalize(direction); // Ensure the direction vector is normalized
    // Return the SH coefficients based on the direction vector
    return vec4(SH_C0, -SH_C1 * direction.y, SH_C1 * direction.z, -SH_C1 * direction.x);
}

// SH constants for a cosine-weighted lobe function
#define SH_cosLobe_C0 0.886226925 // SH constant for the zeroth cosine lobe coefficient (sqrt(pi)/2)
#define SH_cosLobe_C1 1.02332671  // SH constant for the first-order cosine lobe coefficients (sqrt(pi/3))

// Evaluate the SH coefficients for a cosine-weighted lobe (used for diffuse lighting)
vec4 SH_EvaluateCosineLobe(vec3 direction) {
    direction = normalize(direction); // Ensure the direction vector is normalized
    // Return the cosine lobe SH coefficients based on the direction vector
    return vec4(SH_cosLobe_C0, -SH_cosLobe_C1 * direction.y, SH_cosLobe_C1 * direction.z, -SH_cosLobe_C1 * direction.x);
}

// Structure to store Spherical Harmonics coefficients for each color channel
struct SH_Coefficients {
    vec4 red, green, blue; // SH coefficients for the red, green, and blue color channels
};

#endif
