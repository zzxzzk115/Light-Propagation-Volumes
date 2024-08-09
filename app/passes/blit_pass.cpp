#include "passes/blit_pass.hpp"

BlitPass::BlitPass(vgfw::renderer::RenderContext& rc) : BasePass(rc)
{
    auto program = m_RenderContext.createGraphicsProgram(vgfw::utils::readFileAllText("shaders/fullscreen.vert"),
                                                         vgfw::utils::readFileAllText("shaders/blit.frag"));

    m_Pipeline = vgfw::renderer::GraphicsPipeline::Builder {}
                     .setShaderProgram(program)
                     .setDepthStencil({
                         .depthTest  = false,
                         .depthWrite = false,
                     })
                     .setRasterizerState({
                         .polygonMode = vgfw::renderer::PolygonMode::eFill,
                         .cullMode    = vgfw::renderer::CullMode::eBack,
                         .scissorTest = false,
                     })
                     .setBlendState(0,
                                    {
                                        .enabled   = true,
                                        .srcColor  = vgfw::renderer::BlendFactor::eOne,
                                        .destColor = vgfw::renderer::BlendFactor::eOne,
                                    })
                     .build();
}

BlitPass::~BlitPass() { m_RenderContext.destroy(m_Pipeline); }

FrameGraphResource BlitPass::addToGraph(FrameGraph& fg, FrameGraphResource target, FrameGraphResource source)
{
    VGFW_PROFILE_FUNCTION

    assert(target != source);

    fg.addCallbackPass(
        "Blit Pass",
        [&](FrameGraph::Builder& builder, auto&) {
            builder.read(source);

            target = builder.write(target);
        },
        [=, this](const auto&, FrameGraphPassResources& resources, void* ctx) {
            NAMED_DEBUG_MARKER("Blit Pass");
            VGFW_PROFILE_GL("Blit Pass");
            VGFW_PROFILE_NAMED_SCOPE("Blit Pass");

            const auto extent = resources.getDescriptor<vgfw::renderer::framegraph::FrameGraphTexture>(target).extent;
            const vgfw::renderer::RenderingInfo renderingInfo {
                .area             = {.extent = extent},
                .colorAttachments = {{
                    .image = vgfw::renderer::framegraph::getTexture(resources, target),
                }},
            };
            auto&      rc          = *static_cast<vgfw::renderer::RenderContext*>(ctx);
            const auto framebuffer = rc.beginRendering(renderingInfo);
            rc.bindGraphicsPipeline(m_Pipeline)
                .bindTexture(0, vgfw::renderer::framegraph::getTexture(resources, source))
                .drawFullScreenTriangle()
                .endRendering(framebuffer);
        });

    return target;
}