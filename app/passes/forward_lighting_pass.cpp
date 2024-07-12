#include "passes/forward_lighting_pass.hpp"
#include "FrameGraphResource.hpp"
#include "pass_resource/camera_data.hpp"
#include "pass_resource/light_data.hpp"
#include "vgfw.hpp"

ForwardLightingPass::ForwardLightingPass(vgfw::renderer::RenderContext& rc) : BasePass(rc) {}

ForwardLightingPass::~ForwardLightingPass()
{
    for (auto& [_, pipeline] : m_Pipelines)
    {
        m_RenderContext.destroy(pipeline);
    }
}

FrameGraphResource ForwardLightingPass::addToGraph(FrameGraph&                                       fg,
                                                   FrameGraphBlackboard&                             blackboard,
                                                   const vgfw::renderer::Extent2D&                   resolution,
                                                   const std::vector<vgfw::resource::MeshPrimitive>& meshPrimitives)
{
    VGFW_PROFILE_FUNCTION

    const auto [cameraUniform] = blackboard.get<CameraData>();
    const auto [lightUniform]  = blackboard.get<LightData>();

    struct Data
    {
        FrameGraphResource sceneColorHDR;
        FrameGraphResource depth;
    };
    const auto& forwardLighting = fg.addCallbackPass<Data>(
        "Forward Lighting Pass",
        [&](FrameGraph::Builder& builder, Data& data) {
            VGFW_PROFILE_NAMED_SCOPE("Forward Lighting Pass Setup");
            builder.read(cameraUniform);
            builder.read(lightUniform);

            data.sceneColorHDR = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "SceneColorHDR", {.extent = resolution, .format = vgfw::renderer::PixelFormat::eRGB8_UNorm});
            data.sceneColorHDR = builder.write(data.sceneColorHDR);

            data.depth = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "Depth", {.extent = resolution, .format = vgfw::renderer::PixelFormat::eDepth24});
            data.depth = builder.write(data.depth);
        },
        [=, &meshPrimitives, this](const Data& data, FrameGraphPassResources& resources, void* ctx) {
            NAMED_DEBUG_MARKER("Forward Lighting Pass");
            VGFW_PROFILE_GL("Forward Lighting Pass");
            VGFW_PROFILE_NAMED_SCOPE("Forward Lighting Pass");

            auto& rc = *static_cast<vgfw::renderer::RenderContext*>(ctx);

            constexpr glm::vec4 kBlackColor {0.0f};
            constexpr float     kFarPlane {1.0f};

            const vgfw::renderer::RenderingInfo renderingInfo {
                .area             = {.extent = resolution},
                .colorAttachments = {{
                    .image      = vgfw::renderer::framegraph::getTexture(resources, data.sceneColorHDR),
                    .clearValue = kBlackColor,
                }},
                .depthAttachment  = vgfw::renderer::AttachmentInfo {.image = vgfw::renderer::framegraph::getTexture(
                                                                       resources, data.depth),
                                                                    .clearValue = kFarPlane},
            };

            const auto framebuffer = rc.beginRendering(renderingInfo);

            // Draw
            for (const auto& meshPrimitive : meshPrimitives)
            {
                rc.bindGraphicsPipeline(getPipeline(*meshPrimitive.vertexFormat))
                    .bindUniformBuffer(0, vgfw::renderer::framegraph::getBuffer(resources, cameraUniform))
                    .bindUniformBuffer(1, vgfw::renderer::framegraph::getBuffer(resources, lightUniform))
                    .bindMeshPrimitiveMaterialBuffer(2, meshPrimitive)
                    .bindMeshPrimitiveTextures(0, meshPrimitive)
                    .drawMeshPrimitive(meshPrimitive);
            }

            rc.endRendering(framebuffer);
        });

    return forwardLighting.sceneColorHDR;
}

vgfw::renderer::GraphicsPipeline& ForwardLightingPass::getPipeline(const vgfw::renderer::VertexFormat& vertexFormat)
{
    VGFW_PROFILE_FUNCTION
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

vgfw::renderer::GraphicsPipeline ForwardLightingPass::createPipeline(const vgfw::renderer::VertexFormat& vertexFormat)
{
    auto vertexArrayObject = m_RenderContext.getVertexArray(vertexFormat.getAttributes());

    auto program = m_RenderContext.createGraphicsProgram(vgfw::utils::readFileAllText("shaders/geometry.vert"),
                                                         vgfw::utils::readFileAllText("shaders/forward_lighting.frag"));

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