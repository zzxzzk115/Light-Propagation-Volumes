#pragma once

#include "base_pass.hpp"

class FxaaPass : public BasePass
{
public:
    explicit FxaaPass(vgfw::renderer::RenderContext& rc);
    ~FxaaPass();

    FrameGraphResource addToGraph(FrameGraph& fg, FrameGraphResource input);

private:
    vgfw::renderer::GraphicsPipeline m_Pipeline;
};