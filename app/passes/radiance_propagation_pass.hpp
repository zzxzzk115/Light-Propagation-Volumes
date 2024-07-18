#pragma once

#include "base_pass.hpp"

#include "grid3d.hpp"
#include "pass_resource/radiance_data.hpp"

class RadiancePropagationPass : public BasePass
{
public:
    explicit RadiancePropagationPass(vgfw::renderer::RenderContext& rc);
    ~RadiancePropagationPass();

    RadianceData addToGraph(FrameGraph& fg, const RadianceData& radianceData, const Grid3D& grid, uint32_t iteration);

private:
    vgfw::renderer::GraphicsPipeline m_Pipeline;
};