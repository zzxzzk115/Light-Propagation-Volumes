#pragma once

#include "base_pass.hpp"

#include "grid3d.hpp"
#include "pass_resource/radiance_data.hpp"
#include "pass_resource/reflective_shadow_map_data.hpp"

class RadianceInjectionPass : public BasePass
{
public:
    explicit RadianceInjectionPass(vgfw::renderer::RenderContext& rc);
    ~RadianceInjectionPass();

    RadianceData addToGraph(FrameGraph& fg, const ReflectiveShadowMapData& rsmData, const Grid3D& grid);

private:
    vgfw::renderer::GraphicsPipeline m_Pipeline;
};