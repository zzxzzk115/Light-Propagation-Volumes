#include "passes/final_composition_pass.hpp"
#include "pass_resource/gbuffer_data.hpp"
#include "pass_resource/hbao_data.hpp"
#include "pass_resource/reflective_shadow_map_data.hpp"
#include "pass_resource/scene_color_data.hpp"
#include "pass_resource/ssr_data.hpp"
#include "vgfw.hpp"

FinalCompositionPass::FinalCompositionPass(vgfw::renderer::RenderContext& rc) : BasePass(rc)
{
    auto program = m_RenderContext.createGraphicsProgram(vgfw::utils::readFileAllText("shaders/fullscreen.vert"),
                                                         vgfw::utils::readFileAllText("shaders/final.frag"));

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

FinalCompositionPass::~FinalCompositionPass() { m_RenderContext.destroy(m_Pipeline); }

void FinalCompositionPass::compose(FrameGraph& fg, FrameGraphBlackboard& blackboard, RenderSettings& settings)
{
    VGFW_PROFILE_FUNCTION

    FrameGraphResource output {-1};

    const auto defaultExtent =
        fg.getDescriptor<vgfw::renderer::framegraph::FrameGraphTexture>(blackboard.get<SceneColorData>().hdr).extent;

    switch (settings.renderTarget)
    {
        case RenderTarget::eFinal:
            if (settings.enableFXAA)
            {
                output = blackboard.get<SceneColorData>().aa;
            }
            else
            {
                output = blackboard.get<SceneColorData>().ldr;
            }
            break;

        case RenderTarget::eRSMPosition:
            output = blackboard.get<ReflectiveShadowMapData>().position;
            break;

        case RenderTarget::eRSMNormal:
            output = blackboard.get<ReflectiveShadowMapData>().normal;
            break;

        case RenderTarget::eRSMFlux:
            output = blackboard.get<ReflectiveShadowMapData>().flux;
            break;

        case RenderTarget::eGPosition:
            output = blackboard.get<GBufferData>().position;
            break;

        case RenderTarget::eGNormal:
            output = blackboard.get<GBufferData>().normal;
            break;

        case RenderTarget::eGAlbedo:
            output = blackboard.get<GBufferData>().albedo;
            break;

        case RenderTarget::eGEmissive:
            output = blackboard.get<GBufferData>().emissive;
            break;

        case RenderTarget::eGMetallicRoughnessAO:
            output = blackboard.get<GBufferData>().metallicRoughnessAO;
            break;

        case RenderTarget::eHBAO:
            if (settings.enableHBAO)
            {
                output = blackboard.get<HBAOData>().hbao;
            }
            break;

        case RenderTarget::eSceneColorHDR:
            output = blackboard.get<SceneColorData>().hdr;
            break;

        case RenderTarget::eSSR:
            if (settings.enableSSR)
            {
                output = blackboard.get<SSRData>().ssr;
            }
            break;

        case RenderTarget::eSceneColorLDR:
            output = blackboard.get<SceneColorData>().ldr;
            break;
    }

    fg.addCallbackPass(
        "Final Composition Pass",
        [&](FrameGraph::Builder& builder, auto&) {
            if (output != -1)
            {
                builder.read(output);
            }
            builder.setSideEffect();
        },
        [=, this](const auto&, FrameGraphPassResources& resources, void* ctx) {
            NAMED_DEBUG_MARKER("Final Composition Pass");
            VGFW_PROFILE_GL("Final Composition Pass");
            VGFW_PROFILE_NAMED_SCOPE("Final Composition Pass");

            const auto extent =
                output == -1 ? defaultExtent :
                               resources.getDescriptor<vgfw::renderer::framegraph::FrameGraphTexture>(output).extent;

            auto& rc = *static_cast<vgfw::renderer::RenderContext*>(ctx);

            rc.beginRendering({.extent = extent}, glm::vec4 {0.0f});

            if (output != -1)
            {
                rc.bindGraphicsPipeline(m_Pipeline)
                    .bindTexture(0, vgfw::renderer::framegraph::getTexture(resources, output))
                    .drawFullScreenTriangle();
            }
        });
}