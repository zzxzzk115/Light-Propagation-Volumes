#pragma once

#include "base_pass.hpp"

class BaseGeometryPass : public BasePass
{
public:
    explicit BaseGeometryPass(vgfw::renderer::RenderContext& rc);
    virtual ~BaseGeometryPass();

protected:
    void                                     setTransform(const glm::mat4& modelMatrix);
    vgfw::renderer::GraphicsPipeline&        getPipeline(const vgfw::renderer::VertexFormat&);
    virtual vgfw::renderer::GraphicsPipeline createPipeline(const vgfw::renderer::VertexFormat&) = 0;

protected:
    std::unordered_map<size_t, vgfw::renderer::GraphicsPipeline> m_Pipelines;
};