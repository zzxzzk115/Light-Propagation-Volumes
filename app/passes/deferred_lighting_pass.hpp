#pragma once

#include "passes/base_pass.hpp"

#include "grid3d.hpp"
#include "render_settings.hpp"

class DeferredLightingPass : public BasePass
{
public:
    explicit DeferredLightingPass(vgfw::renderer::RenderContext& rc);
    ~DeferredLightingPass();

    void addToGraph(FrameGraph&           fg,
                    FrameGraphBlackboard& blackboard,
                    const glm::mat4&      lightViewProjection,
                    const Grid3D&         grid,
                    RenderSettings&       settings);

private:
    vgfw::renderer::GraphicsPipeline m_Pipeline;
};