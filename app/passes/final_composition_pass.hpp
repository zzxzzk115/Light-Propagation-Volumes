#pragma once

#include "base_pass.hpp"
#include "render_settings.hpp"
#include "render_target.hpp"

class FinalCompositionPass : public BasePass
{
public:
    explicit FinalCompositionPass(vgfw::renderer::RenderContext& rc);
    ~FinalCompositionPass();

    void compose(FrameGraph& fg, FrameGraphBlackboard& blackboard, RenderTarget renderTarget, RenderSettings& settings);

private:
    vgfw::renderer::GraphicsPipeline m_Pipeline;
};