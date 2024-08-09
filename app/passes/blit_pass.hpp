#pragma once

#include "base_pass.hpp"

class BlitPass : public BasePass
{
public:
    explicit BlitPass(vgfw::renderer::RenderContext& rc);
    ~BlitPass();

    FrameGraphResource addToGraph(FrameGraph& fg, FrameGraphResource target, FrameGraphResource source);

private:
    vgfw::renderer::GraphicsPipeline m_Pipeline;
};