#include "passes/reflective_shadow_map_pass.hpp"

#include "pass_resource/light_data.hpp"
#include "pass_resource/reflective_shadow_map_data.hpp"

#include "lpv_config.hpp"

ReflectiveShadowMapPass::ReflectiveShadowMapPass(vgfw::renderer::RenderContext& rc) : BasePass(rc) {}

ReflectiveShadowMapPass::~ReflectiveShadowMapPass()
{
    for (auto& [_, pipeline] : m_Pipelines)
    {
        m_RenderContext.destroy(pipeline);
    }
}

void ReflectiveShadowMapPass::addToGraph(FrameGraph&                                       fg,
                                         FrameGraphBlackboard&                             blackboard,
                                         const glm::mat4&                                  lightViewProjection,
                                         const std::vector<vgfw::resource::MeshPrimitive>& meshPrimitives)
{
    VGFW_PROFILE_FUNCTION

    const auto [lightUniform] = blackboard.get<LightData>();

    constexpr auto kExtent = vgfw::renderer::Extent2D {kRSMResolution, kRSMResolution};

    blackboard.add<ReflectiveShadowMapData>() = fg.addCallbackPass<ReflectiveShadowMapData>(
        "ReflectiveShadowMap Pass",
        [&](FrameGraph::Builder& builder, ReflectiveShadowMapData& data) {
            builder.read(lightUniform);

            data.position = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "RSM-Position",
                {
                    .extent = kExtent,
                    .format = vgfw::renderer::PixelFormat::eRGB16F,
                    .wrap   = vgfw::renderer::WrapMode::eClampToOpaqueBlack,
                    .filter = vgfw::renderer::TexelFilter::eNearest,
                });
            data.normal = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "RSM-Normal",
                {
                    .extent = kExtent,
                    .format = vgfw::renderer::PixelFormat::eRGB16F,
                    .wrap   = vgfw::renderer::WrapMode::eClampToOpaqueBlack,
                    .filter = vgfw::renderer::TexelFilter::eNearest,
                });
            data.flux = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "RSM-Flux",
                {
                    .extent = kExtent,
                    .format = vgfw::renderer::PixelFormat::eRGB16F,
                    .wrap   = vgfw::renderer::WrapMode::eClampToOpaqueBlack,
                    .filter = vgfw::renderer::TexelFilter::eNearest,
                });

            data.depth = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "RSM-Depth",
                {
                    .extent = kExtent,
                    .format = vgfw::renderer::PixelFormat::eDepth24,
                });

            data.position = builder.write(data.position);
            data.normal   = builder.write(data.normal);
            data.flux     = builder.write(data.flux);
            data.depth    = builder.write(data.depth);
        },
        [=, &meshPrimitives, this](const ReflectiveShadowMapData& data, FrameGraphPassResources& resources, void* ctx) {
            NAMED_DEBUG_MARKER("ReflectiveShadowMap Pass");
            VGFW_PROFILE_GL("ReflectiveShadowMap Pass");
            VGFW_PROFILE_NAMED_SCOPE("ReflectiveShadowMap Pass");

            constexpr float     kFarPlane {1.0f};
            constexpr glm::vec4 kBlackColor {0.0f};

            const vgfw::renderer::RenderingInfo renderingInfo {
                .area = {.extent = kExtent},
                .colorAttachments =
                    {
                        {
                            .image      = vgfw::renderer::framegraph::getTexture(resources, data.position),
                            .clearValue = kBlackColor,
                        },
                        {
                            .image      = vgfw::renderer::framegraph::getTexture(resources, data.normal),
                            .clearValue = kBlackColor,
                        },
                        {
                            .image      = vgfw::renderer::framegraph::getTexture(resources, data.flux),
                            .clearValue = kBlackColor,
                        },
                    },
                .depthAttachment =
                    vgfw::renderer::AttachmentInfo {
                        .image      = vgfw::renderer::framegraph::getTexture(resources, data.depth),
                        .clearValue = kFarPlane,
                    },

            };

            auto& rc = *static_cast<vgfw::renderer::RenderContext*>(ctx);

            const auto framebuffer = rc.beginRendering(renderingInfo);

            for (const auto& meshPrimitive : meshPrimitives)
            {
                rc.bindGraphicsPipeline(getPipeline(*meshPrimitive.vertexFormat))
                    .setUniformMat4("uTransform.model", meshPrimitive.modelMatrix)
                    .setUniformMat4("uTransform.viewProjection", lightViewProjection)
                    .bindUniformBuffer(1, vgfw::renderer::framegraph::getBuffer(resources, lightUniform))
                    .bindMeshPrimitiveMaterialBuffer(2, meshPrimitive)
                    .bindMeshPrimitiveTextures(0, meshPrimitive)
                    .drawMeshPrimitive(meshPrimitive);
            }

            rc.endRendering(framebuffer);
        });
}

vgfw::renderer::GraphicsPipeline& ReflectiveShadowMapPass::getPipeline(const vgfw::renderer::VertexFormat& vertexFormat)
{
    size_t hash = vertexFormat.getHash();

    vgfw::renderer::GraphicsPipeline* passPipeline = nullptr;

    if (const auto it = m_Pipelines.find(hash); it != m_Pipelines.cend())
        passPipeline = &it->second;

    if (!passPipeline)
    {
        auto pipeline = createPipeline(vertexFormat);

        const auto& it = m_Pipelines.insert_or_assign(hash, std::move(pipeline)).first;
        passPipeline   = &it->second;
    }

    return *passPipeline;
}

vgfw::renderer::GraphicsPipeline
ReflectiveShadowMapPass::createPipeline(const vgfw::renderer::VertexFormat& vertexFormat)
{
    auto vertexArrayObject = m_RenderContext.getVertexArray(vertexFormat.getAttributes());

    auto program =
        m_RenderContext.createGraphicsProgram(vgfw::utils::readFileAllText("shaders/geometry.vert"),
                                              vgfw::utils::readFileAllText("shaders/reflective_shadow_map.frag"));

    return vgfw::renderer::GraphicsPipeline::Builder {}
        .setDepthStencil({
            .depthTest      = true,
            .depthWrite     = true,
            .depthCompareOp = vgfw::renderer::CompareOp::eLessOrEqual,
        })
        .setRasterizerState({
            .polygonMode = vgfw::renderer::PolygonMode::eFill,
            .cullMode    = vgfw::renderer::CullMode::eBack,
            .scissorTest = false,
        })
        .setVAO(vertexArrayObject)
        .setShaderProgram(program)
        .build();
}