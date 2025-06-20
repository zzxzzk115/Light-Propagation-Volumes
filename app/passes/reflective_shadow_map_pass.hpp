#pragma once

#include "passes/base_geometry_pass.hpp"

class ReflectiveShadowMapPass : public BaseGeometryPass
{
public:
    explicit ReflectiveShadowMapPass(vgfw::renderer::RenderContext& rc);
    ~ReflectiveShadowMapPass() = default;

    void addToGraph(FrameGraph&                                       fg,
                    FrameGraphBlackboard&                             blackboard,
                    const glm::mat4&                                  lightViewProjection,
                    const std::vector<vgfw::resource::MeshPrimitive>& meshPrimitives);

private:
    virtual vgfw::renderer::GraphicsPipeline createPipeline(const vgfw::renderer::VertexFormat&) override final;
};