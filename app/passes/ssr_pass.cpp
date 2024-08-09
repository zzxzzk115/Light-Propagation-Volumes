#include "passes/ssr_pass.hpp"
#include "pass_resource/camera_data.hpp"
#include "pass_resource/gbuffer_data.hpp"
#include "pass_resource/scene_color_data.hpp"
#include "pass_resource/ssr_data.hpp"

SsrPass::SsrPass(vgfw::renderer::RenderContext& rc) : BasePass(rc)
{
    auto program = m_RenderContext.createGraphicsProgram(vgfw::utils::readFileAllText("shaders/fullscreen.vert"),
                                                         vgfw::utils::readFileAllText("shaders/ssr.frag"));

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

SsrPass::~SsrPass() { m_RenderContext.destroy(m_Pipeline); }

FrameGraphResource SsrPass::addToGraph(FrameGraph& fg, FrameGraphBlackboard& blackboard, RenderSettings& settings)
{
    const auto [cameraUniform] = blackboard.get<CameraData>();

    const auto& gBuffer    = blackboard.get<GBufferData>();
    const auto  extent     = fg.getDescriptor<vgfw::renderer::framegraph::FrameGraphTexture>(gBuffer.depth).extent;
    const auto& sceneColor = blackboard.get<SceneColorData>();

    const auto& pass = fg.addCallbackPass<SSRData>(
        "SSR Pass",
        [&](FrameGraph::Builder& builder, SSRData& data) {
            builder.read(cameraUniform);

            builder.read(gBuffer.depth);
            builder.read(gBuffer.normal);
            builder.read(gBuffer.metallicRoughnessAO);
            builder.read(sceneColor.hdr);

            data.ssr = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "SSR", {.extent = extent, .format = vgfw::renderer::PixelFormat::eRGB16F});
            data.ssr = builder.write(data.ssr);
        },
        [=, this](const SSRData& data, FrameGraphPassResources& resources, void* ctx) {
            NAMED_DEBUG_MARKER("SSR Pass");
            VGFW_PROFILE_GL("SSR Pass");
            VGFW_PROFILE_NAMED_SCOPE("SSR Pass");

            const vgfw::renderer::RenderingInfo renderingInfo {
                .area             = {.extent = extent},
                .colorAttachments = {{
                    .image      = vgfw::renderer::framegraph::getTexture(resources, data.ssr),
                    .clearValue = glm::vec4 {0.0f},
                }},
            };

            auto&      rc          = *static_cast<vgfw::renderer::RenderContext*>(ctx);
            const auto framebuffer = rc.beginRendering(renderingInfo);
            rc.bindGraphicsPipeline(m_Pipeline)
                .bindUniformBuffer(0, vgfw::renderer::framegraph::getBuffer(resources, cameraUniform))
                .setUniform1f("reflectionFactor", settings.reflectionFactor)
                .bindTexture(0, vgfw::renderer::framegraph::getTexture(resources, gBuffer.depth))
                .bindTexture(1, vgfw::renderer::framegraph::getTexture(resources, gBuffer.normal))
                .bindTexture(2, vgfw::renderer::framegraph::getTexture(resources, gBuffer.metallicRoughnessAO))
                .bindTexture(3, vgfw::renderer::framegraph::getTexture(resources, sceneColor.hdr))
                .drawFullScreenTriangle()
                .endRendering(framebuffer);
        });

    return pass.ssr;
}