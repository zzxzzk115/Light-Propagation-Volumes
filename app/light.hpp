#pragma once

#include <glm/glm.hpp>

struct DirectionalLight
{
    glm::vec3 direction = glm::normalize(glm::vec3(1.0f, -1.0f, 0.0f));
    float     intensity = 1.0f;
    glm::vec3 color     = {1, 1, 1};
};