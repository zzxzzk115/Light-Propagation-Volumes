#include "passes/hbao_pass.hpp"
#include "pass_resource/camera_data.hpp"
#include "pass_resource/gbuffer_data.hpp"
#include "pass_resource/hbao_data.hpp"

#include <random>

HbaoPass::HbaoPass(vgfw::renderer::RenderContext& rc) : BasePass(rc)
{
    generateNoiseTexture();

    auto program = m_RenderContext.createGraphicsProgram(vgfw::utils::readFileAllText("shaders/fullscreen.vert"),
                                                         vgfw::utils::readFileAllText("shaders/hbao.frag"));

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

HbaoPass::~HbaoPass() { m_RenderContext.destroy(m_Pipeline).destroy(m_Noise); }

void HbaoPass::addToGraph(FrameGraph& fg, FrameGraphBlackboard& blackboard, const HBAOProperties& properties)
{
    VGFW_PROFILE_FUNCTION

    const auto [cameraUniform] = blackboard.get<CameraData>();

    const auto& gBuffer = blackboard.get<GBufferData>();
    const auto  extent  = fg.getDescriptor<vgfw::renderer::framegraph::FrameGraphTexture>(gBuffer.depth).extent;

    blackboard.add<HBAOData>() = fg.addCallbackPass<HBAOData>(
        "HBAO Pass",
        [&](FrameGraph::Builder& builder, HBAOData& data) {
            builder.read(cameraUniform);
            builder.read(gBuffer.depth);
            builder.read(gBuffer.normal);

            data.hbao = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "HBAO Map", {.extent = extent, .format = vgfw::renderer::PixelFormat::eR8_UNorm});
            data.hbao = builder.write(data.hbao);
        },
        [=, this](const HBAOData& data, FrameGraphPassResources& resources, void* ctx) {
            NAMED_DEBUG_MARKER("HBAO Pass");
            VGFW_PROFILE_GL("HBAO Pass");
            VGFW_PROFILE_NAMED_SCOPE("HBAO Pass");

            const vgfw::renderer::RenderingInfo renderingInfo {
                .area             = {.extent = extent},
                .colorAttachments = {{
                    .image      = vgfw::renderer::framegraph::getTexture(resources, data.hbao),
                    .clearValue = glm::vec4 {1.0f},
                }},
            };

            auto&      rc          = *static_cast<vgfw::renderer::RenderContext*>(ctx);
            const auto framebuffer = rc.beginRendering(renderingInfo);
            rc.bindGraphicsPipeline(m_Pipeline)
                .bindUniformBuffer(0, vgfw::renderer::framegraph::getBuffer(resources, cameraUniform))
                .setUniform1f("uHBAO_radius", properties.radius)
                .setUniform1f("uHBAO_bias", properties.bias)
                .setUniform1f("uHBAO_intensity", properties.intensity)
                .setUniform1f("uHBAO_negInvRadius2", -1.0 / (properties.radius * properties.radius))
                .setUniform1i("uHBAO_maxRadiusPixels", properties.maxRadiusPixels)
                .setUniform1i("uHBAO_stepCount", properties.stepCount)
                .setUniform1i("uHBAO_directionCount", properties.directionCount)
                .bindTexture(0, vgfw::renderer::framegraph::getTexture(resources, gBuffer.depth))
                .bindTexture(1, vgfw::renderer::framegraph::getTexture(resources, gBuffer.normal))
                .bindTexture(2, m_Noise)
                .drawFullScreenTriangle()
                .endRendering(framebuffer);
        });
}

void HbaoPass::generateNoiseTexture()
{
    constexpr auto kSize = 4u;

    std::uniform_real_distribution<float> dist {0.0, 1.0};
    std::random_device                    rd {};
    std::default_random_engine            g {rd()};
    std::vector<glm::vec3>                hbaoNoise;
    std::generate_n(std::back_inserter(hbaoNoise), kSize * kSize, [&] { return glm::vec3 {dist(g), dist(g), 0.0f}; });

    m_Noise = m_RenderContext.createTexture2D({kSize, kSize}, vgfw::renderer::PixelFormat::eRGB16F);
    m_RenderContext
        .setupSampler(m_Noise,
                      {
                          .minFilter    = vgfw::renderer::TexelFilter::eLinear,
                          .mipmapMode   = vgfw::renderer::MipmapMode::eNone,
                          .magFilter    = vgfw::renderer::TexelFilter::eLinear,
                          .addressModeS = vgfw::renderer::SamplerAddressMode::eRepeat,
                          .addressModeT = vgfw::renderer::SamplerAddressMode::eRepeat,
                      })
        .upload(m_Noise,
                0,
                {kSize, kSize},
                {
                    .format   = GL_RGB,
                    .dataType = GL_FLOAT,
                    .pixels   = hbaoNoise.data(),
                });
}