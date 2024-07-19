#pragma once

#include "passes/base_geometry_pass.hpp"

#include "camera.hpp"

class GBufferPass : public BaseGeometryPass
{
public:
    explicit GBufferPass(vgfw::renderer::RenderContext& rc);
    ~GBufferPass() = default;

    void addToGraph(FrameGraph&                                       fg,
                    FrameGraphBlackboard&                             blackboard,
                    const vgfw::renderer::Extent2D&                   resolution,
                    const Camera&                                     camera,
                    const std::vector<vgfw::resource::MeshPrimitive>& meshPrimitives);

private:
    virtual vgfw::renderer::GraphicsPipeline createPipeline(const vgfw::renderer::VertexFormat&) override final;
};