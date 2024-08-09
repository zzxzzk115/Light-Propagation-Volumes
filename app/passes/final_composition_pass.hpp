#pragma once

#include "base_pass.hpp"
#include "render_settings.hpp"

class FinalCompositionPass : public BasePass
{
public:
    explicit FinalCompositionPass(vgfw::renderer::RenderContext& rc);
    ~FinalCompositionPass();

    void compose(FrameGraph& fg, FrameGraphBlackboard& blackboard, RenderSettings& settings);

private:
    vgfw::renderer::GraphicsPipeline m_Pipeline;
};