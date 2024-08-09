#pragma once

#include "base_pass.hpp"
#include "render_settings.hpp"

class SsrPass : public BasePass
{
public:
    explicit SsrPass(vgfw::renderer::RenderContext& rc);
    ~SsrPass();

    FrameGraphResource addToGraph(FrameGraph& fg, FrameGraphBlackboard& blackboard, RenderSettings& settings);

private:
    vgfw::renderer::GraphicsPipeline m_Pipeline;
};