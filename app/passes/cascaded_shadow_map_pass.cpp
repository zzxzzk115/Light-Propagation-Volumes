#include "passes/cascaded_shadow_map_pass.hpp"
#include "pass_resource/shadow_data.hpp"

const uint32_t kShadowMapSize = 2048;
const uint32_t kNumCascades   = 1;

// clang-format off
#if GLM_CONFIG_CLIP_CONTROL & GLM_CLIP_CONTROL_ZO_BIT
    const glm::mat4 kBiasMatrix{
        0.5, 0.0, 0.0, 0.0,
        0.0, 0.5, 0.0, 0.0,
        0.0, 0.0, 1.0, 0.0,
        0.5, 0.5, 0.0, 1.0
    };
#else
    const glm::mat4 kBiasMatrix{
        0.5, 0.0, 0.0, 0.0,
        0.0, 0.5, 0.0, 0.0,
        0.0, 0.0, 0.5, 0.0,
        0.5, 0.5, 0.5, 1.0
    };
#endif
// clang-format on

struct CascadesUniform
{
    glm::vec4 splitDepth;
    glm::mat4 lightSpaceMatrices[kNumCascades];
};

void uploadCascades(FrameGraph&                                    fg,
                    FrameGraphBlackboard&                          blackboard,
                    std::vector<vgfw::renderer::shadow::Cascade>&& cascades)
{
    auto& cascadedUniformBuffer = blackboard.get<ShadowData>().cascadedUniformBuffer;

    fg.addCallbackPass(
        "Upload Shadow Cascades",
        [&](FrameGraph::Builder& builder, auto&) { cascadedUniformBuffer = builder.write(cascadedUniformBuffer); },
        [=, cascades = std::move(cascades)](const auto&, FrameGraphPassResources& resources, void* ctx) {
            NAMED_DEBUG_MARKER("Upload Shadow Cascades");
            VGFW_PROFILE_GL("Upload Shadow Cascades");
            VGFW_PROFILE_NAMED_SCOPE("Upload Shadow Cascades");

            CascadesUniform cascadesUniform {};
            for (uint32_t i = 0; i < cascades.size(); ++i)
            {
                cascadesUniform.splitDepth[i]         = cascades[i].splitDepth;
                cascadesUniform.lightSpaceMatrices[i] = kBiasMatrix * cascades[i].viewProjection;
            }
            static_cast<vgfw::renderer::RenderContext*>(ctx)->upload(
                vgfw::renderer::framegraph::getBuffer(resources, cascadedUniformBuffer),
                0,
                sizeof(CascadesUniform),
                &cascadesUniform);
        });
}

CascadedShadowMapPass::CascadedShadowMapPass(vgfw::renderer::RenderContext& rc) : BaseGeometryPass(rc)
{
    m_CascadedUniformBuffer = rc.createBuffer(sizeof(CascadesUniform));
}

CascadedShadowMapPass::~CascadedShadowMapPass() { m_RenderContext.destroy(m_CascadedUniformBuffer); }

void CascadedShadowMapPass::addToGraph(FrameGraph&                                       fg,
                                       FrameGraphBlackboard&                             blackboard,
                                       const Camera&                                     camera,
                                       const DirectionalLight&                           light,
                                       const std::vector<vgfw::resource::MeshPrimitive>& meshPrimitives)
{
    VGFW_PROFILE_FUNCTION

    auto& shadowMapData = blackboard.add<ShadowData>();
    shadowMapData.cascadedUniformBuffer =
        vgfw::renderer::framegraph::importBuffer(fg, "Cascaded Uniform Buffer", &m_CascadedUniformBuffer);

    auto cascades = vgfw::renderer::shadow::buildCascades(camera.zNear,
                                                          camera.zFar,
                                                          camera.data.projection * camera.data.view,
                                                          light.direction,
                                                          kNumCascades,
                                                          0.94f,
                                                          kShadowMapSize);

    std::optional<FrameGraphResource> cascadedShadowMaps;
    for (uint32_t i = 0; i < cascades.size(); ++i)
    {
        const auto& lightViewProj = cascades[i].viewProjection;
        cascadedShadowMaps        = addCascadePass(fg, cascadedShadowMaps, lightViewProj, std::move(meshPrimitives), i);
    }
    assert(cascadedShadowMaps);
    shadowMapData.cascadedShadowMaps = *cascadedShadowMaps;
    uploadCascades(fg, blackboard, std::move(cascades));
}

FrameGraphResource
CascadedShadowMapPass::addCascadePass(FrameGraph&                                       fg,
                                      std::optional<FrameGraphResource>                 cascadedShadowMaps,
                                      const glm::mat4&                                  lightViewProjection,
                                      const std::vector<vgfw::resource::MeshPrimitive>& meshPrimitives,
                                      uint32_t                                          cascadeIdx)
{
    assert(cascadeIdx < kNumCascades);
    const auto name = "CSM #" + std::to_string(cascadeIdx);

    struct Data
    {
        FrameGraphResource output;
    };
    const auto& pass = fg.addCallbackPass<Data>(
        name,
        [&](FrameGraph::Builder& builder, Data& data) {
            if (cascadeIdx == 0)
            {
                assert(!cascadedShadowMaps);
                cascadedShadowMaps = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                    "CascadedShadowMaps",
                    {
                        .extent        = {kShadowMapSize, kShadowMapSize},
                        .layers        = kNumCascades,
                        .format        = vgfw::renderer::PixelFormat::eDepth24,
                        .shadowSampler = true,
                    });
            }

            data.output = builder.write(*cascadedShadowMaps);
        },
        [=, this, &meshPrimitives](const Data& data, FrameGraphPassResources& resources, void* ctx) {
            NAMED_DEBUG_MARKER(name);
            VGFW_PROFILE_GL("CSM");
            VGFW_PROFILE_NAMED_SCOPE("CSM");

            constexpr float                     kFarPlane {1.0f};
            const vgfw::renderer::RenderingInfo renderingInfo {
                .area = {.extent = {kShadowMapSize, kShadowMapSize}},
                .depthAttachment =
                    vgfw::renderer::AttachmentInfo {
                        .image      = vgfw::renderer::framegraph::getTexture(resources, data.output),
                        .layer      = cascadeIdx,
                        .clearValue = kFarPlane,
                    },
            };
            auto&      rc          = *static_cast<vgfw::renderer::RenderContext*>(ctx);
            const auto framebuffer = rc.beginRendering(renderingInfo);

            for (const auto& meshPrimitive : meshPrimitives)
            {
                rc.bindGraphicsPipeline(getPipeline(*meshPrimitive.vertexFormat))
                    .setUniformMat4("uTransform.model", meshPrimitive.modelMatrix)
                    .setUniformMat4("uTransform.viewProjection", lightViewProjection)
                    .drawMeshPrimitive(meshPrimitive);
            }
            rc.endRendering(framebuffer);
        });

    return pass.output;
}

vgfw::renderer::GraphicsPipeline CascadedShadowMapPass::createPipeline(const vgfw::renderer::VertexFormat& vertexFormat)
{
    auto vertexArrayObject = m_RenderContext.getVertexArray(vertexFormat.getAttributes());

    auto program = m_RenderContext.createGraphicsProgram(vgfw::utils::readFileAllText("shaders/geometry.vert"),
                                                         vgfw::utils::readFileAllText("shaders/shadow_map.frag"));

    return vgfw::renderer::GraphicsPipeline::Builder {}
        .setDepthStencil({
            .depthTest      = true,
            .depthWrite     = true,
            .depthCompareOp = vgfw::renderer::CompareOp::eLessOrEqual,
        })
        .setRasterizerState({
            .polygonMode = vgfw::renderer::PolygonMode::eFill,
            .cullMode    = vgfw::renderer::CullMode::eFront, // avoid Peter-Planning
            .scissorTest = false,
        })
        .setVAO(vertexArrayObject)
        .setShaderProgram(program)
        .build();
}