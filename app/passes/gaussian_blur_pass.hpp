#pragma once

#include "base_pass.hpp"

class GaussianBlurPass : public BasePass
{
public:
    explicit GaussianBlurPass(vgfw::renderer::RenderContext& rc);
    ~GaussianBlurPass();

    FrameGraphResource addToGraph(FrameGraph& fg, FrameGraphResource input, float scale);

private:
    FrameGraphResource addToGraph(FrameGraph& fg, FrameGraphResource input, float scale, bool horizontal);

private:
    vgfw::renderer::GraphicsPipeline m_Pipeline;
};