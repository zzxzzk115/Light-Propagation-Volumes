#pragma once

#include "base_pass.hpp"

class HbaoPass : public BasePass
{
public:
    explicit HbaoPass(vgfw::renderer::RenderContext& rc);
    ~HbaoPass();

    void addToGraph(FrameGraph& fg, FrameGraphBlackboard& blackboard);

private:
    void generateNoiseTexture();

private:
    vgfw::renderer::GraphicsPipeline m_Pipeline;
    vgfw::renderer::Texture          m_Noise;
};