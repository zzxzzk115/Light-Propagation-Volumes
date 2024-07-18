#include "passes/radiance_propagation_pass.hpp"

#include "lpv_config.hpp"

RadiancePropagationPass::RadiancePropagationPass(vgfw::renderer::RenderContext& rc) : BasePass(rc)
{
    auto program =
        m_RenderContext.createGraphicsProgram(vgfw::utils::readFileAllText("shaders/radiance_propagation.vert"),
                                              vgfw::utils::readFileAllText("shaders/radiance_propagation.frag"),
                                              vgfw::utils::readFileAllText("shaders/radiance_propagation.geom"));

    m_Pipeline = vgfw::renderer::GraphicsPipeline::Builder {}
                     .setDepthStencil({.depthTest = false, .depthWrite = false})
                     .setRasterizerState({.cullMode = vgfw::renderer::CullMode::eNone})
                     .setBlendState(0, kAdditiveBlending)
                     .setBlendState(1, kAdditiveBlending)
                     .setBlendState(2, kAdditiveBlending)
                     .setShaderProgram(program)
                     .build();
}

RadiancePropagationPass::~RadiancePropagationPass() { m_RenderContext.destroy(m_Pipeline); }

RadianceData RadiancePropagationPass::addToGraph(FrameGraph&         fg,
                                                 const RadianceData& radianceData,
                                                 const Grid3D&       grid,
                                                 uint32_t            iteration)
{
    VGFW_PROFILE_FUNCTION

    const auto name   = "RadiancePropagation #" + std::to_string(iteration);
    const auto extent = vgfw::renderer::Extent2D {.width = grid.size.x, .height = grid.size.y};

    const auto data = fg.addCallbackPass<RadianceData>(
        name,
        [&](FrameGraph::Builder& builder, RadianceData& data) {
            builder.read(radianceData.r);
            builder.read(radianceData.g);
            builder.read(radianceData.b);

            data.r = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "SH-R",
                {
                    .extent   = extent,
                    .depth    = grid.size.z,
                    .format   = vgfw::renderer::PixelFormat::eRGBA16F,
                    .wrapMode = vgfw::renderer::WrapMode::eClampToOpaqueBlack,
                    .filter   = vgfw::renderer::TexelFilter::eLinear,
                });
            data.g = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "SH-G",
                {
                    .extent   = extent,
                    .depth    = grid.size.z,
                    .format   = vgfw::renderer::PixelFormat::eRGBA16F,
                    .wrapMode = vgfw::renderer::WrapMode::eClampToOpaqueBlack,
                    .filter   = vgfw::renderer::TexelFilter::eLinear,
                });
            data.b = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "SH-B",
                {
                    .extent   = extent,
                    .depth    = grid.size.z,
                    .format   = vgfw::renderer::PixelFormat::eRGBA16F,
                    .wrapMode = vgfw::renderer::WrapMode::eClampToOpaqueBlack,
                    .filter   = vgfw::renderer::TexelFilter::eLinear,
                });

            data.r = builder.write(data.r);
            data.g = builder.write(data.g);
            data.b = builder.write(data.b);
        },
        [=, this](const RadianceData& data, FrameGraphPassResources& resources, void* ctx) {
            NAMED_DEBUG_MARKER("RadiancePropagation Pass");
            VGFW_PROFILE_GL("RadiancePropagation Pass");
            VGFW_PROFILE_NAMED_SCOPE("RadiancePropagation Pass");

            constexpr glm::vec4 kBlackColor {0.0f};

            const vgfw::renderer::RenderingInfo renderingInfo {
                .area = {.extent = extent},
                .colorAttachments =
                    {
                        {
                            .image      = vgfw::renderer::framegraph::getTexture(resources, data.r),
                            .clearValue = kBlackColor,
                        },
                        {
                            .image      = vgfw::renderer::framegraph::getTexture(resources, data.g),
                            .clearValue = kBlackColor,
                        },
                        {
                            .image      = vgfw::renderer::framegraph::getTexture(resources, data.b),
                            .clearValue = kBlackColor,
                        },
                    },
            };

            auto&      rc          = *static_cast<vgfw::renderer::RenderContext*>(ctx);
            const auto framebuffer = rc.beginRendering(renderingInfo);

            rc.bindGraphicsPipeline(m_Pipeline)
                .bindTexture(0, vgfw::renderer::framegraph::getTexture(resources, radianceData.r))
                .bindTexture(1, vgfw::renderer::framegraph::getTexture(resources, radianceData.g))
                .bindTexture(2, vgfw::renderer::framegraph::getTexture(resources, radianceData.b))
                .setUniformVec3("uInjection.gridSize", grid.size)
                .draw(std::nullopt,
                      std::nullopt,
                      vgfw::renderer::GeometryInfo {
                          .topology    = vgfw::renderer::PrimitiveTopology::ePointList,
                          .numVertices = kNumVPL,
                      });

            rc.endRendering(framebuffer);
        });

    return data;
}