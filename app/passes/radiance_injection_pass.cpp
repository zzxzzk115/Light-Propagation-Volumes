#include "radiance_injection_pass.hpp"

#include "lpv_config.hpp"

RadianceInjectionPass::RadianceInjectionPass(vgfw::renderer::RenderContext& rc) : BasePass(rc)
{
    auto program =
        m_RenderContext.createGraphicsProgram(vgfw::utils::readFileAllText("shaders/radiance_injection.vert"),
                                              vgfw::utils::readFileAllText("shaders/radiance_injection.frag"),
                                              vgfw::utils::readFileAllText("shaders/radiance_injection.geom"));

    m_Pipeline = vgfw::renderer::GraphicsPipeline::Builder {}
                     .setDepthStencil({.depthTest = false, .depthWrite = false})
                     .setRasterizerState({.cullMode = vgfw::renderer::CullMode::eNone})
                     .setBlendState(0, kAdditiveBlending)
                     .setBlendState(1, kAdditiveBlending)
                     .setBlendState(2, kAdditiveBlending)
                     .setShaderProgram(program)
                     .build();
}

RadianceInjectionPass::~RadianceInjectionPass() { m_RenderContext.destroy(m_Pipeline); }

RadianceData
RadianceInjectionPass::addToGraph(FrameGraph& fg, const ReflectiveShadowMapData& rsmData, const Grid3D& grid)
{
    VGFW_PROFILE_FUNCTION

    const auto extent = vgfw::renderer::Extent2D {.width = grid.size.x, .height = grid.size.y};

    auto data = fg.addCallbackPass<RadianceData>(
        "RadianceInjection",
        [&](FrameGraph::Builder& builder, RadianceData& data) {
            builder.read(rsmData.position);
            builder.read(rsmData.normal);
            builder.read(rsmData.flux);

            data.r = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "SH-R",
                {
                    .extent   = extent,
                    .depth    = grid.size.z,
                    .format   = vgfw::renderer::PixelFormat::eRGBA16F,
                    .wrapMode = vgfw::renderer::WrapMode::eClampToOpaqueBlack,
                    .filter   = vgfw::renderer::TexelFilter::eNearest,
                });
            data.g = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "SH-G",
                {
                    .extent   = extent,
                    .depth    = grid.size.z,
                    .format   = vgfw::renderer::PixelFormat::eRGBA16F,
                    .wrapMode = vgfw::renderer::WrapMode::eClampToOpaqueBlack,
                    .filter   = vgfw::renderer::TexelFilter::eNearest,
                });
            data.b = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "SH-B",
                {
                    .extent   = extent,
                    .depth    = grid.size.z,
                    .format   = vgfw::renderer::PixelFormat::eRGBA16F,
                    .wrapMode = vgfw::renderer::WrapMode::eClampToOpaqueBlack,
                    .filter   = vgfw::renderer::TexelFilter::eNearest,
                });

            data.r = builder.write(data.r);
            data.g = builder.write(data.g);
            data.b = builder.write(data.b);
        },
        [=, this](const RadianceData& data, FrameGraphPassResources& resources, void* ctx) {
            NAMED_DEBUG_MARKER("RadianceInjection Pass");
            VGFW_PROFILE_GL("RadianceInjection Pass");
            VGFW_PROFILE_NAMED_SCOPE("RadianceInjection Pass");

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
                .bindTexture(0, vgfw::renderer::framegraph::getTexture(resources, rsmData.position))
                .bindTexture(1, vgfw::renderer::framegraph::getTexture(resources, rsmData.normal))
                .bindTexture(2, vgfw::renderer::framegraph::getTexture(resources, rsmData.flux))
                .setUniform1i("uInjection.rsmResolution", kRSMResolution)
                .setUniformVec3("uInjection.gridAABBMin", grid.aabb.min)
                .setUniformVec3("uInjection.gridSize", grid.size)
                .setUniform1f("uInjection.gridCellSize", grid.cellSize)
                .draw({},
                      {},
                      vgfw::renderer::GeometryInfo {
                          .topology    = vgfw::renderer::PrimitiveTopology::ePointList,
                          .numVertices = kNumVPL,
                      });

            rc.endRendering(framebuffer);
        });

    return data;
}