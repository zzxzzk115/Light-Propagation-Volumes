#pragma once

#include "base_pass.hpp"

class BloomPass : public BasePass
{
public:
    explicit BloomPass(vgfw::renderer::RenderContext& rc);
    ~BloomPass();

    FrameGraphResource
    addToGraph(FrameGraph& fg, FrameGraphResource sceneColor, FrameGraphResource sceneColorBrightBlur, float factor);

private:
    vgfw::renderer::GraphicsPipeline m_Pipeline;
};