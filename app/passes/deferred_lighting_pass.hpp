#pragma once

#include "passes/base_pass.hpp"

#include "camera.hpp"
#include "grid3d.hpp"
#include "visual_mode.hpp"

class DeferredLightingPass : public BasePass
{
public:
    explicit DeferredLightingPass(vgfw::renderer::RenderContext& rc);
    ~DeferredLightingPass();

    FrameGraphResource addToGraph(FrameGraph&           fg,
                                  FrameGraphBlackboard& blackboard,
                                  const glm::mat4&      lightViewProjection,
                                  const Grid3D&         grid,
                                  VisualMode            visualMode);

private:
    vgfw::renderer::GraphicsPipeline m_Pipeline;
};