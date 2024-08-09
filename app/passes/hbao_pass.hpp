#pragma once

#include "base_pass.hpp"

struct HBAOProperties
{
    float radius {400.0f};
    float bias {0.05f};
    float intensity {5.0f};
    int   maxRadiusPixels {256};
    int   stepCount {4};
    int   directionCount {8};
};

class HbaoPass : public BasePass
{
public:
    explicit HbaoPass(vgfw::renderer::RenderContext& rc);
    ~HbaoPass();

    void addToGraph(FrameGraph& fg, FrameGraphBlackboard& blackboard, const HBAOProperties& properties);

private:
    void generateNoiseTexture();

private:
    vgfw::renderer::GraphicsPipeline m_Pipeline;
    vgfw::renderer::Texture          m_Noise;
};