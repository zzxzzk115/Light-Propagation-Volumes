#include "passes/deferred_lighting_pass.hpp"
#include "pass_resource/camera_data.hpp"
#include "pass_resource/gbuffer_data.hpp"
#include "pass_resource/hbao_data.hpp"
#include "pass_resource/light_data.hpp"
#include "pass_resource/radiance_data.hpp"
#include "pass_resource/shadow_data.hpp"

DeferredLightingPass::DeferredLightingPass(vgfw::renderer::RenderContext& rc) : BasePass(rc)
{
    auto program =
        m_RenderContext.createGraphicsProgram(vgfw::utils::readFileAllText("shaders/fullscreen.vert"),
                                              vgfw::utils::readFileAllText("shaders/deferred_lighting.frag"));

    m_Pipeline = vgfw::renderer::GraphicsPipeline::Builder {}
                     .setDepthStencil({
                         .depthTest  = false,
                         .depthWrite = false,
                     })
                     .setRasterizerState({
                         .polygonMode = vgfw::renderer::PolygonMode::eFill,
                         .cullMode    = vgfw::renderer::CullMode::eBack,
                         .scissorTest = false,
                     })
                     .setShaderProgram(program)
                     .build();
}

DeferredLightingPass::~DeferredLightingPass() { m_RenderContext.destroy(m_Pipeline); }

FrameGraphResource DeferredLightingPass::addToGraph(FrameGraph&           fg,
                                                    FrameGraphBlackboard& blackboard,
                                                    const glm::mat4&      lightViewProjection,
                                                    const Grid3D&         grid,
                                                    VisualMode            visualMode,
                                                    RenderSettings&       settings)
{
    VGFW_PROFILE_FUNCTION

    const auto [cameraUniform] = blackboard.get<CameraData>();
    const auto [lightUniform]  = blackboard.get<LightData>();
    const auto gBuffer         = blackboard.get<GBufferData>();
    const auto radianceData    = blackboard.get<RadianceData>();
    const auto shadowData      = blackboard.get<ShadowData>();

    HBAOData hbaoData {};
    if (settings.enableHBAO)
    {
        hbaoData = blackboard.get<HBAOData>();
    }

    const auto extent = fg.getDescriptor<vgfw::renderer::framegraph::FrameGraphTexture>(gBuffer.depth).extent;

    struct Data
    {
        FrameGraphResource sceneColorHDR;
    };
    const auto& forwardLighting = fg.addCallbackPass<Data>(
        "Deferred Lighting Pass",
        [&](FrameGraph::Builder& builder, Data& data) {
            VGFW_PROFILE_NAMED_SCOPE("Deferred Lighting Pass Setup");
            builder.read(cameraUniform);
            builder.read(lightUniform);

            builder.read(gBuffer.position);
            builder.read(gBuffer.normal);
            builder.read(gBuffer.albedo);
            builder.read(gBuffer.emissive);
            builder.read(gBuffer.metallicRoughnessAO);
            builder.read(gBuffer.depth);

            builder.read(radianceData.r);
            builder.read(radianceData.g);
            builder.read(radianceData.b);

            builder.read(shadowData.cascadedShadowMaps);
            builder.read(shadowData.cascadedUniformBuffer);

            if (settings.enableHBAO)
            {
                builder.read(hbaoData.hbao);
            }

            data.sceneColorHDR = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "SceneColorHDR", {.extent = extent, .format = vgfw::renderer::PixelFormat::eRGB16F});
            data.sceneColorHDR = builder.write(data.sceneColorHDR);
        },
        [=, this](const Data& data, FrameGraphPassResources& resources, void* ctx) {
            NAMED_DEBUG_MARKER("Deferred Lighting Pass");
            VGFW_PROFILE_GL("Deferred Lighting Pass");
            VGFW_PROFILE_NAMED_SCOPE("Deferred Lighting Pass");

            auto& rc = *static_cast<vgfw::renderer::RenderContext*>(ctx);

            constexpr glm::vec4 kSceneBGColor {0.529, 0.808, 0.922, 1.0};
            constexpr float     kFarPlane {1.0f};

            const vgfw::renderer::RenderingInfo renderingInfo {
                .area             = {.extent = extent},
                .colorAttachments = {{
                    .image      = vgfw::renderer::framegraph::getTexture(resources, data.sceneColorHDR),
                    .clearValue = kSceneBGColor,
                }},
            };

            const auto framebuffer = rc.beginRendering(renderingInfo);

            rc.bindGraphicsPipeline(m_Pipeline)
                .setUniform1ui("uVisualMode", static_cast<uint32_t>(visualMode))
                .setUniformVec3("uInjection.gridAABBMin", grid.aabb.min)
                .setUniformVec3("uInjection.gridSize", grid.size)
                .setUniform1f("uInjection.gridCellSize", grid.cellSize)
                .setUniformMat4("uLightVP", lightViewProjection)
                .setUniform1i("uSettings.enableHBAO", settings.enableHBAO)
                .setUniform1i("uSettings.enableSSR", settings.enableSSR)
                .setUniform1i("uSettings.enableTAA", settings.enableTAA)
                .setUniform1i("uSettings.enableBloom", settings.enableBloom)
                .bindUniformBuffer(0, vgfw::renderer::framegraph::getBuffer(resources, cameraUniform))
                .bindUniformBuffer(1, vgfw::renderer::framegraph::getBuffer(resources, lightUniform))
                .bindUniformBuffer(2,
                                   vgfw::renderer::framegraph::getBuffer(resources, shadowData.cascadedUniformBuffer))
                .bindTexture(0, vgfw::renderer::framegraph::getTexture(resources, gBuffer.position))
                .bindTexture(1, vgfw::renderer::framegraph::getTexture(resources, gBuffer.normal))
                .bindTexture(2, vgfw::renderer::framegraph::getTexture(resources, gBuffer.albedo))
                .bindTexture(3, vgfw::renderer::framegraph::getTexture(resources, gBuffer.emissive))
                .bindTexture(4, vgfw::renderer::framegraph::getTexture(resources, gBuffer.metallicRoughnessAO))
                .bindTexture(5, vgfw::renderer::framegraph::getTexture(resources, gBuffer.depth))
                .bindTexture(6, vgfw::renderer::framegraph::getTexture(resources, shadowData.cascadedShadowMaps))
                .bindTexture(7, vgfw::renderer::framegraph::getTexture(resources, radianceData.r))
                .bindTexture(8, vgfw::renderer::framegraph::getTexture(resources, radianceData.g))
                .bindTexture(9, vgfw::renderer::framegraph::getTexture(resources, radianceData.b));

            if (settings.enableHBAO)
            {
                rc.bindTexture(10, vgfw::renderer::framegraph::getTexture(resources, hbaoData.hbao));
            }

            rc.drawFullScreenTriangle().endRendering(framebuffer);
        });

    return forwardLighting.sceneColorHDR;
}