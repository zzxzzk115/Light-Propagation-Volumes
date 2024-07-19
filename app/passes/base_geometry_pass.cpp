#include "passes/base_geometry_pass.hpp"

BaseGeometryPass::BaseGeometryPass(vgfw::renderer::RenderContext& rc) : BasePass(rc) {}

BaseGeometryPass::~BaseGeometryPass()
{
    for (auto& [_, pipeline] : m_Pipelines)
    {
        m_RenderContext.destroy(pipeline);
    }
}

void BaseGeometryPass::setTransform(const glm::mat4& modelMatrix)
{
    m_RenderContext.setUniformMat4("uTransform.model", modelMatrix);
}

vgfw::renderer::GraphicsPipeline& BaseGeometryPass::getPipeline(const vgfw::renderer::VertexFormat& vertexFormat)
{
    size_t hash = vertexFormat.getHash();

    vgfw::renderer::GraphicsPipeline* passPipeline = nullptr;

    if (const auto it = m_Pipelines.find(hash); it != m_Pipelines.cend())
        passPipeline = &it->second;

    if (!passPipeline)
    {
        auto pipeline = createPipeline(vertexFormat);

        const auto& it = m_Pipelines.insert_or_assign(hash, std::move(pipeline)).first;
        passPipeline   = &it->second;
    }

    return *passPipeline;
}