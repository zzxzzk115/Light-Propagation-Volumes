#pragma once

#include "passes/base_geometry_pass.hpp"

#include "camera.hpp"
#include "light.hpp"

class CascadedShadowMapPass : public BaseGeometryPass
{
public:
    explicit CascadedShadowMapPass(vgfw::renderer::RenderContext& rc);
    ~CascadedShadowMapPass();

    void addToGraph(FrameGraph&                                       fg,
                    FrameGraphBlackboard&                             blackboard,
                    const Camera&                                     camera,
                    const DirectionalLight&                           light,
                    const std::vector<vgfw::resource::MeshPrimitive>& meshPrimitives);

private:
    FrameGraphResource addCascadePass(FrameGraph&                                       fg,
                                      std::optional<FrameGraphResource>                 cascadedShadowMaps,
                                      const glm::mat4&                                  lightViewProjection,
                                      const std::vector<vgfw::resource::MeshPrimitive>& meshPrimitives,
                                      uint32_t                                          cascadeIdx);

    virtual vgfw::renderer::GraphicsPipeline createPipeline(const vgfw::renderer::VertexFormat&) override final;

private:
    vgfw::renderer::Buffer m_CascadedUniformBuffer;
};