#include "grid3d.hpp"

#include "lpv_config.hpp"

Grid3D::Grid3D(const vgfw::math::AABB& aabb) : aabb {aabb}
{
    const auto extent = aabb.getExtent();
    cellSize          = vgfw::math::max3(extent) / kLPVResolution;
    size              = glm::uvec3 {extent / cellSize + 0.5f};
}