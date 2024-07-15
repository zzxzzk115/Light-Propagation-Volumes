#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec4 aTangent;

layout(location = 0) out vec2 vTexCoords;
layout(location = 1) out vec3 vFragPos;
layout(location = 2) out mat3 vTBN;

struct Transform {
    mat4 model;
    mat4 viewProjection;
};

uniform Transform uTransform;

void main() {
    vec4 fragPos = uTransform.model * vec4(aPos, 1.0);
    vFragPos = fragPos.xyz;
    vTexCoords = aTexCoords;
    vTBN = mat3(aTangent.xyz, cross(aTangent.xyz, aNormal) * aTangent.w, aNormal);
    gl_Position = uTransform.viewProjection * fragPos;
}