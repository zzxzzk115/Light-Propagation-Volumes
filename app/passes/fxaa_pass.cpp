#include "passes/fxaa_pass.hpp"

FxaaPass::FxaaPass(vgfw::renderer::RenderContext& rc) : BasePass(rc)
{
    auto program = m_RenderContext.createGraphicsProgram(vgfw::utils::readFileAllText("shaders/fullscreen.vert"),
                                                         vgfw::utils::readFileAllText("shaders/fxaa.frag"));

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
                     .build();
}

FxaaPass::~FxaaPass() { m_RenderContext.destroy(m_Pipeline); }

FrameGraphResource FxaaPass::addToGraph(FrameGraph& fg, FrameGraphResource input)
{
    VGFW_PROFILE_FUNCTION

    const auto extent = fg.getDescriptor<vgfw::renderer::framegraph::FrameGraphTexture>(input).extent;

    struct Data
    {
        FrameGraphResource output;
    };
    const auto& pass = fg.addCallbackPass<Data>(
        "FXAA Pass",
        [&](FrameGraph::Builder& builder, Data& data) {
            builder.read(input);

            data.output = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "FXAA processed SceneColor", {.extent = extent, .format = vgfw::renderer::PixelFormat::eRGB8_UNorm});
            data.output = builder.write(data.output);
        },
        [=, this](const Data& data, FrameGraphPassResources& resources, void* ctx) {
            NAMED_DEBUG_MARKER("FXAA Pass");
            VGFW_PROFILE_GL("FXAA Pass");
            VGFW_PROFILE_NAMED_SCOPE("FXAA Pass");

            const vgfw::renderer::RenderingInfo renderingInfo {
                .area             = {.extent = extent},
                .colorAttachments = {{
                    .image = vgfw::renderer::framegraph::getTexture(resources, data.output),
                }},
            };

            auto&      rc          = *static_cast<vgfw::renderer::RenderContext*>(ctx);
            const auto framebuffer = rc.beginRendering(renderingInfo);
            rc.bindGraphicsPipeline(m_Pipeline)
                .bindTexture(0, vgfw::renderer::framegraph::getTexture(resources, input))
                .setUniformVec2("uResolution", glm::vec2(extent.width, extent.height))
                .drawFullScreenTriangle()
                .endRendering(framebuffer);
        });

    return pass.output;
}