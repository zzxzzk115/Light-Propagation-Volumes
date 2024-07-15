#pragma once

#include "base_pass.hpp"

class ReflectiveShadowMapPass : public BasePass
{
public:
    explicit ReflectiveShadowMapPass(vgfw::renderer::RenderContext& rc);
    ~ReflectiveShadowMapPass();

    void addToGraph(FrameGraph&                                       fg,
                    FrameGraphBlackboard&                             blackboard,
                    const glm::mat4&                                  lightViewProjection,
                    const std::vector<vgfw::resource::MeshPrimitive>& meshPrimitives);

private:
    vgfw::renderer::GraphicsPipeline& getPipeline(const vgfw::renderer::VertexFormat&);
    vgfw::renderer::GraphicsPipeline  createPipeline(const vgfw::renderer::VertexFormat&);

private:
    std::unordered_map<size_t, vgfw::renderer::GraphicsPipeline> m_Pipelines;
};