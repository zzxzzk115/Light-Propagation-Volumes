#define VGFW_IMPLEMENTATION
#define IMGUI_DEFINE_MATH_OPERATORS
#include "vgfw.hpp"

#include "uniforms/camera_uniform.hpp"
#include "uniforms/light_uniform.hpp"

#include "pass_resource/hbao_data.hpp"
#include "pass_resource/radiance_data.hpp"
#include "pass_resource/reflective_shadow_map_data.hpp"
#include "pass_resource/scene_color_data.hpp"
#include "pass_resource/ssr_data.hpp"
#include "passes/blit_pass.hpp"
#include "passes/bloom_pass.hpp"

#include "passes/cascaded_shadow_map_pass.hpp"
#include "passes/deferred_lighting_pass.hpp"
#include "passes/final_composition_pass.hpp"
#include "passes/fxaa_pass.hpp"
#include "passes/gaussian_blur_pass.hpp"
#include "passes/gbuffer_pass.hpp"
#include "passes/hbao_pass.hpp"
#include "passes/radiance_injection_pass.hpp"
#include "passes/radiance_propagation_pass.hpp"
#include "passes/reflective_shadow_map_pass.hpp"
#include "passes/ssr_pass.hpp"
#include "passes/tonemapping_pass.hpp"

#include "grid3d.hpp"
#include "lpv_config.hpp"
#include "render_settings.hpp"

int main()
try
{
    // Init VGFW
    if (!vgfw::init())
    {
        std::cerr << "Failed to initialize VGFW" << std::endl;
        return -1;
    }

    // Create a window instance
    auto window = vgfw::window::create({.title = "Light Propagation Volumes"});

    // Init renderer
    vgfw::renderer::init({.window = window});

    // Get render context
    auto& rc = vgfw::renderer::getRenderContext();

    // Create transient resources
    vgfw::renderer::framegraph::TransientResources transientResources(rc);

    // Load model
    vgfw::resource::Model sponza {};
    if (!vgfw::io::loadModel("assets/models/Sponza/glTF/Sponza.gltf", sponza, rc, glm::vec3(0.035f)))
    {
        return -1;
    }

    DirectionalLight light {};
    light.direction = {0.000, -0.984, 0.177};
    light.intensity = 10.0f;

    // Camera properties
    Camera camera {};
    camera.data.position = {30, 20, -1.5};
    camera.yaw           = -90.0f;
    camera.updateData(window);

    // Create a texture sampler
    auto sampler = rc.createSampler({.maxAnisotropy = 8});

    vgfw::time::TimePoint lastTime = vgfw::time::Clock::now();

    // Define render passes
    CascadedShadowMapPass   csmPass(rc);
    ReflectiveShadowMapPass rsmPass(rc);
    RadianceInjectionPass   radianceInjectionPass(rc);
    RadiancePropagationPass radiancePropagationPass(rc);
    GBufferPass             gBufferPass(rc);
    HbaoPass                hbaoPass(rc);
    GaussianBlurPass        gaussianBlurPass(rc);
    DeferredLightingPass    deferredLightingPass(rc);
    BloomPass               bloomPass(rc);
    SsrPass                 ssrPass(rc);
    BlitPass                blitPass(rc);
    TonemappingPass         tonemappingPass(rc);
    FxaaPass                fxaaPass(rc);
    FinalCompositionPass    finalCompositionPass(rc);

    // Get scene grid
    Grid3D sceneGrid {sponza.aabb};

    // Render settings
    RenderSettings settings {};

    // Main loop
    while (!window->shouldClose())
    {
        VGFW_PROFILE_NAMED_SCOPE("Main loop");

        vgfw::time::TimePoint currentTime = vgfw::time::Clock::now();
        vgfw::time::Duration  deltaTime   = currentTime - lastTime;
        lastTime                          = currentTime;

        float dt = deltaTime.count();

        window->onTick();

        camera.update(window, dt);

        FrameGraph           fg;
        FrameGraphBlackboard blackboard;

        uploadCameraUniform(fg, blackboard, camera.data);
        uploadLightUniform(fg, blackboard, light);

        // Build Shadow map cascades
        csmPass.addToGraph(fg, blackboard, camera, light, sponza.meshPrimitives);

        // Build RSM cascade
        auto rsmCascade = vgfw::renderer::shadow::buildCascades(camera.zNear,
                                                                camera.zFar,
                                                                camera.data.projection * camera.data.view,
                                                                light.direction,
                                                                1,
                                                                1,
                                                                kRSMResolution);
        auto rsmLightVP = rsmCascade[0].viewProjection;

        // RSM pass
        rsmPass.addToGraph(fg, blackboard, rsmLightVP, sponza.meshPrimitives);

        // Radiance Injection Pass
        auto rsmData      = blackboard.get<ReflectiveShadowMapData>();
        auto radianceData = radianceInjectionPass.addToGraph(fg, rsmData, sceneGrid);

        // Radiance Propagation Pass
        std::optional<RadianceData> propagatedRadiance;
        for (auto i = 0; i < settings.lpvIteration; ++i)
            propagatedRadiance = radiancePropagationPass.addToGraph(
                fg, propagatedRadiance ? *propagatedRadiance : radianceData, sceneGrid, i);
        blackboard.add<RadianceData>(propagatedRadiance ? *propagatedRadiance : radianceData);

        // GBuffer pass
        gBufferPass.addToGraph(fg,
                               blackboard,
                               {.width = window->getWidth(), .height = window->getHeight()},
                               camera,
                               sponza.meshPrimitives);

        if (settings.enableHBAO)
        {
            // HBAO pass
            hbaoPass.addToGraph(fg, blackboard, settings.hbaoProperties);

            // 2-pass Gaussian blur
            auto& hbao = blackboard.get<HBAOData>().hbao;
            hbao       = gaussianBlurPass.addToGraph(fg, hbao, 1.0f);
        }

        // Deferred Lighting pass
        deferredLightingPass.addToGraph(fg, blackboard, rsmLightVP, sceneGrid, settings);
        auto& sceneColor = blackboard.get<SceneColorData>();

        if (settings.enableBloom)
        {
            // Blur bright
            sceneColor.bright = gaussianBlurPass.addToGraph(fg, sceneColor.bright, 1.0f);

            // Bloom pass
            sceneColor.hdr = bloomPass.addToGraph(fg, sceneColor.hdr, sceneColor.bright, settings.bloomFactor);
        }

        if (settings.enableSSR)
        {
            // SSR pass
            const auto ssr = ssrPass.addToGraph(fg, blackboard, settings);
            blackboard.add<SSRData>(ssr);
            sceneColor.hdr = blitPass.addToGraph(fg, sceneColor.hdr, ssr);
        }

        // Tone-mapping pass
        sceneColor.ldr = tonemappingPass.addToGraph(fg, sceneColor.hdr);

        if (settings.enableFXAA)
        {
            // FXAA pass
            sceneColor.aa = fxaaPass.addToGraph(fg, sceneColor.ldr);
        }

        // Final composition pass
        finalCompositionPass.compose(fg, blackboard, settings);

        {
            VGFW_PROFILE_NAMED_SCOPE("Compile FrameGraph");
            fg.compile();
        }

        vgfw::renderer::beginFrame();

        {
            VGFW_PROFILE_NAMED_SCOPE("Execute FrameGraph");
            fg.execute(&rc, &transientResources);
        }

#ifndef NDEBUG
        {
            VGFW_PROFILE_NAMED_SCOPE("Export FrameGraph");
            // Built in graphviz writer.
            std::ofstream {"DebugFrameGraph.dot"} << fg;
        }
#endif

        transientResources.update(dt);

        {
            VGFW_PROFILE_NAMED_SCOPE("ImGui");
            const auto windowFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
                                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
                                     ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

            const ImVec2 pad {10.0f, 10.0f};
            const auto*  viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos + pad, ImGuiCond_Always);

            ImGui::SetNextWindowBgAlpha(0.35f);
            if (ImGui::Begin("MetricsOverlay", nullptr, windowFlags))
            {
                const auto frameRate = ImGui::GetIO().Framerate;
                ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / frameRate, frameRate);
            }
            ImGui::End();

            ImGui::Begin("Render settings");
            ImGui::SliderFloat("Camera FOV", &camera.fov, 1.0f, 179.0f);
            ImGui::Text("Press CAPSLOCK to toggle the camera (W/A/S/D/Q/E + Mouse)");

            ImGui::DragFloat3("Light Direction", glm::value_ptr(light.direction), 0.01f);
            ImGui::DragFloat("Light Intensity", &light.intensity, 0.5f, 0.0f, 100.0f);
            ImGui::ColorEdit3("Light Color", glm::value_ptr(light.color));

            ImGui::Checkbox("Enable HBAO", &settings.enableHBAO);

            if (settings.enableHBAO)
            {
                ImGui::DragFloat("HBAO Radius", &settings.hbaoProperties.radius, 0.005f, 0.0f, 100.0f);
                ImGui::DragFloat("HBAO Bias", &settings.hbaoProperties.bias, 0.005f, 0.0f, 100.0f);
                ImGui::DragFloat("HBAO Intensity", &settings.hbaoProperties.intensity, 0.005f, 0.0f, 100.0f);
                ImGui::DragInt("HBAO MaxRadiusPixels", &settings.hbaoProperties.maxRadiusPixels, 1, 0, 1000);
                ImGui::DragInt("HBAO StepCount", &settings.hbaoProperties.stepCount, 1, 0, 32);
                ImGui::DragInt("HBAO DirectionCount", &settings.hbaoProperties.directionCount, 1, 0, 32);
            }

            ImGui::Checkbox("Enable SSR", &settings.enableSSR);

            if (settings.enableSSR)
            {
                ImGui::DragFloat("Reflection Factor", &settings.reflectionFactor, 0.05f, 0.0f, 100.0f);
            }

            ImGui::Checkbox("Enable FXAA", &settings.enableFXAA);

            ImGui::Checkbox("Enable Bloom", &settings.enableBloom);

            if (settings.enableBloom)
            {
                ImGui::DragFloat("Bloom Factor", &settings.bloomFactor, 0.001f, 0.0f, 5.0f);
            }

            ImGui::SliderInt("LPV Iteration", &settings.lpvIteration, 0, 200);

            const char* visualModeItems[] = {
                "Default",
                "OnlyDirect",
                "OnlyLPV",
            };

            int currentVisualMode = static_cast<int>(settings.visualMode);

            if (ImGui::Combo("Visual Mode", &currentVisualMode, visualModeItems, IM_ARRAYSIZE(visualModeItems)))
            {
                settings.visualMode = static_cast<VisualMode>(currentVisualMode);
            }

            const char* renderTargetItems[] = {"Final",
                                               "RSMPosition",
                                               "RSMNormal",
                                               "RSMFlux",
                                               "G-Normal",
                                               "G-Albedo",
                                               "G-Emissive",
                                               "G-MetallicRoughnessAO",
                                               "HBAO",
                                               "SceneColorHDR",
                                               "SceneColorBright",
                                               "SSR",
                                               "SceneColorLDR"};

            int currentRenderTarget = static_cast<int>(settings.renderTarget);

            if (ImGui::Combo("Render Target", &currentRenderTarget, renderTargetItems, IM_ARRAYSIZE(renderTargetItems)))
            {
                settings.renderTarget = static_cast<RenderTarget>(currentRenderTarget);
            }

            ImGui::End();
        }

        vgfw::renderer::endFrame();

        vgfw::renderer::present();

        VGFW_PROFILE_END_OF_FRAME
    }

    // Cleanup
    vgfw::shutdown();

    return 0;
}
catch (std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return -1;
}