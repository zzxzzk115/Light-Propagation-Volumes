#pragma once

#include "base_pass.hpp"

class ForwardLightingPass : public BasePass
{
public:
    explicit ForwardLightingPass(vgfw::renderer::RenderContext& rc);
    ~ForwardLightingPass();

    FrameGraphResource addToGraph(FrameGraph&                                       fg,
                                  FrameGraphBlackboard&                             blackboard,
                                  const vgfw::renderer::Extent2D&                   resolution,
                                  const std::vector<vgfw::resource::MeshPrimitive>& meshPrimitives);

private:
    vgfw::renderer::GraphicsPipeline& getPipeline(const vgfw::renderer::VertexFormat&);
    vgfw::renderer::GraphicsPipeline  createPipeline(const vgfw::renderer::VertexFormat&);

private:
    std::unordered_map<size_t, vgfw::renderer::GraphicsPipeline> m_Pipelines;
};