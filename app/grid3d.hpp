#pragma once

#include "vgfw.hpp"

struct Grid3D
{
    explicit Grid3D(const vgfw::math::AABB&);

    vgfw::math::AABB aabb;
    glm::uvec3       size;
    float            cellSize;
};