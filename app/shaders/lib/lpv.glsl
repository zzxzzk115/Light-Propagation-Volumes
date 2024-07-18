#ifndef LPV_GLSL
#define LPG_GLSL

struct RadianceInjection {
    vec3 gridAABBMin;
    vec3 gridSize;
    float gridCellSize;
    int rsmResolution;
};

#define SH_C0 0.282094791 // 1 / 2sqrt(pi)
#define SH_C1 0.488602512 // sqrt(3/pi) / 2

vec4 SH_Evaluate(vec3 direction) {
    direction = normalize(direction);
    return vec4(SH_C0, -SH_C1 * direction.y, SH_C1 * direction.z, -SH_C1 * direction.x);
}

#define SH_cosLobe_C0 0.886226925 // sqrt(pi)/2
#define SH_cosLobe_C1 1.02332671  // sqrt(pi/3)

vec4 SH_EvaluateCosineLobe(vec3 direction) {
    direction = normalize(direction);
    return vec4(SH_cosLobe_C0, -SH_cosLobe_C1 * direction.y, SH_cosLobe_C1 * direction.z, -SH_cosLobe_C1 * direction.x);
}

struct SH_Coefficients {
    vec4 red, green, blue;
};

#endif