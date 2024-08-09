#include "passes/gaussian_blur_pass.hpp"

GaussianBlurPass::GaussianBlurPass(vgfw::renderer::RenderContext& rc) : BasePass(rc)
{
    auto program = m_RenderContext.createGraphicsProgram(vgfw::utils::readFileAllText("shaders/fullscreen.vert"),
                                                         vgfw::utils::readFileAllText("shaders/gaussian_blur.frag"));

    m_Pipeline = vgfw::renderer::GraphicsPipeline::Builder {}
                     .setShaderProgram(program)
                     .setDepthStencil({
                         .depthTest  = true,
                         .depthWrite = true,
                     })
                     .setRasterizerState({
                         .polygonMode = vgfw::renderer::PolygonMode::eFill,
                         .cullMode    = vgfw::renderer::CullMode::eBack,
                         .scissorTest = false,
                     })
                     .build();
}

GaussianBlurPass::~GaussianBlurPass() { m_RenderContext.destroy(m_Pipeline); }

FrameGraphResource GaussianBlurPass::addToGraph(FrameGraph& fg, FrameGraphResource input, float scale)
{
    input = addToGraph(fg, input, scale, false);
    return addToGraph(fg, input, scale, true);
}

FrameGraphResource GaussianBlurPass::addToGraph(FrameGraph& fg, FrameGraphResource input, float scale, bool horizontal)
{
    const auto  name = (horizontal ? "Horizontal" : "Vertical") + std::string {" Gaussian Blur Pass"};
    const auto& desc = fg.getDescriptor<vgfw::renderer::framegraph::FrameGraphTexture>(input);

    struct Data
    {
        FrameGraphResource output;
    };
    const auto& pass = fg.addCallbackPass<Data>(
        name,
        [&](FrameGraph::Builder& builder, Data& data) {
            builder.read(input);

            data.output = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "Blurred SceneColor (Gaussian)", {.extent = desc.extent, .format = desc.format});
            data.output = builder.write(data.output);
        },
        [=, this](const Data& data, FrameGraphPassResources& resources, void* ctx) {
            NAMED_DEBUG_MARKER(name);
            VGFW_PROFILE_GL("Gaussian Blur Pass");
            VGFW_PROFILE_NAMED_SCOPE("Gaussian Blur Pass");

            const vgfw::renderer::RenderingInfo renderingInfo {
                .area             = {.extent = desc.extent},
                .colorAttachments = {{
                    .image = vgfw::renderer::framegraph::getTexture(resources, data.output),
                }},
            };

            auto&      rc          = *static_cast<vgfw::renderer::RenderContext*>(ctx);
            const auto framebuffer = rc.beginRendering(renderingInfo);
            rc.bindGraphicsPipeline(m_Pipeline)
                .bindTexture(0, vgfw::renderer::framegraph::getTexture(resources, input))
                .setUniform1f("scale", scale)
                .setUniform1i("horizontal", horizontal)
                .drawFullScreenTriangle()
                .endRendering(framebuffer);
        });

    return pass.output;
}