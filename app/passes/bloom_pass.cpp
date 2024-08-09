#include "passes/bloom_pass.hpp"

BloomPass::BloomPass(vgfw::renderer::RenderContext& rc) : BasePass(rc)
{
    auto program = m_RenderContext.createGraphicsProgram(vgfw::utils::readFileAllText("shaders/fullscreen.vert"),
                                                         vgfw::utils::readFileAllText("shaders/bloom.frag"));

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

BloomPass::~BloomPass() { m_RenderContext.destroy(m_Pipeline); }

FrameGraphResource BloomPass::addToGraph(FrameGraph&        fg,
                                         FrameGraphResource sceneColor,
                                         FrameGraphResource sceneColorBrightBlur,
                                         float              factor)
{
    VGFW_PROFILE_FUNCTION

    const auto extent = fg.getDescriptor<vgfw::renderer::framegraph::FrameGraphTexture>(sceneColor).extent;

    struct Data
    {
        FrameGraphResource output;
    };
    const auto& pass = fg.addCallbackPass<Data>(
        "Bloom Pass",
        [&](FrameGraph::Builder& builder, Data& data) {
            builder.read(sceneColor);
            builder.read(sceneColorBrightBlur);

            data.output = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "Bloom Result", {.extent = extent, .format = vgfw::renderer::PixelFormat::eRGB16F});
            data.output = builder.write(data.output);
        },
        [=, this](const Data& data, FrameGraphPassResources& resources, void* ctx) {
            NAMED_DEBUG_MARKER("Bloom Pass");
            VGFW_PROFILE_GL("Bloom Pass");
            VGFW_PROFILE_NAMED_SCOPE("Bloom Pass");

            const vgfw::renderer::RenderingInfo renderingInfo {
                .area             = {.extent = extent},
                .colorAttachments = {{
                    .image = vgfw::renderer::framegraph::getTexture(resources, data.output),
                }},
            };

            auto&      rc          = *static_cast<vgfw::renderer::RenderContext*>(ctx);
            const auto framebuffer = rc.beginRendering(renderingInfo);
            rc.bindGraphicsPipeline(m_Pipeline)
                .setUniform1f("bloomFactor", factor)
                .bindTexture(0, vgfw::renderer::framegraph::getTexture(resources, sceneColor))
                .bindTexture(1, vgfw::renderer::framegraph::getTexture(resources, sceneColorBrightBlur))
                .drawFullScreenTriangle()
                .endRendering(framebuffer);
        });

    return pass.output;
}