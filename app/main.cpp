#define VGFW_IMPLEMENTATION
#include "vgfw.hpp"

#include "render_target.hpp"

#include "uniforms/camera_uniform.hpp"
#include "uniforms/light_uniform.hpp"

#include "pass_resource/radiance_data.hpp"
#include "pass_resource/reflective_shadow_map_data.hpp"
#include "pass_resource/scene_color_data.hpp"

#include "passes/cascaded_shadow_map_pass.hpp"
#include "passes/deferred_lighting_pass.hpp"
#include "passes/final_composition_pass.hpp"
#include "passes/fxaa_pass.hpp"
#include "passes/gbuffer_pass.hpp"
#include "passes/radiance_injection_pass.hpp"
#include "passes/radiance_propagation_pass.hpp"
#include "passes/reflective_shadow_map_pass.hpp"
#include "passes/tonemapping_pass.hpp"

#include "grid3d.hpp"
#include "lpv_config.hpp"
#include "visual_mode.hpp"

int main()
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
    DeferredLightingPass    deferredLightingPass(rc);
    TonemappingPass         tonemappingPass(rc);
    FxaaPass                fxaaPass(rc);
    FinalCompositionPass    finalCompositionPass(rc);

    // Define render target
    RenderTarget renderTarget = RenderTarget::eFinal;

    // Get scene grid
    Grid3D sceneGrid {sponza.aabb};

    // UI properties
    VisualMode visualMode   = VisualMode::eDefault;
    int        lpvIteration = 12;

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
        for (auto i = 0; i < lpvIteration; ++i)
            propagatedRadiance = radiancePropagationPass.addToGraph(
                fg, propagatedRadiance ? *propagatedRadiance : radianceData, sceneGrid, i);
        blackboard.add<RadianceData>(*propagatedRadiance);

        // GBuffer pass
        gBufferPass.addToGraph(fg,
                               blackboard,
                               {.width = window->getWidth(), .height = window->getHeight()},
                               camera,
                               sponza.meshPrimitives);

        // Deferred Lighting pass
        auto& sceneColor = blackboard.add<SceneColorData>();
        sceneColor.hdr   = deferredLightingPass.addToGraph(fg, blackboard, rsmLightVP, sceneGrid, visualMode);

        // Tone-mapping pass
        sceneColor.ldr = tonemappingPass.addToGraph(fg, sceneColor.hdr);

        // FXAA pass
        sceneColor.aa = fxaaPass.addToGraph(fg, sceneColor.ldr);

        // Final composition pass
        finalCompositionPass.compose(fg, blackboard, renderTarget);

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
            ImGui::Begin("Render settings");
            ImGui::SliderFloat("Camera FOV", &camera.fov, 1.0f, 179.0f);
            ImGui::Text("Press CAPSLOCK to toggle the camera (W/A/S/D/Q/E + Mouse)");

            ImGui::DragFloat3("Light Direction", glm::value_ptr(light.direction), 0.01f);
            ImGui::DragFloat("Light Intensity", &light.intensity, 0.5f, 0.0f, 100.0f);
            ImGui::ColorEdit3("Light Color", glm::value_ptr(light.color));

            ImGui::SliderInt("LPV Iteration", &lpvIteration, 1, 200);

            const char* visualModeItems[] = {
                "Default",
                "OnlyDirect",
                "OnlyLPV",
            };

            int currentVisualMode = static_cast<int>(visualMode);

            if (ImGui::Combo("Visual Mode", &currentVisualMode, visualModeItems, IM_ARRAYSIZE(visualModeItems)))
            {
                visualMode = static_cast<VisualMode>(currentVisualMode);
            }

            const char* renderTargetItems[] = {"Final",
                                               "RSMPosition",
                                               "RSMNormal",
                                               "RSMFlux",
                                               "G-Position",
                                               "G-Normal",
                                               "G-Albedo",
                                               "G-Emissive",
                                               "G-MetallicRoughnessAO",
                                               "SceneColorHDR",
                                               "SceneColorLDR"};

            int currentRenderTarget = static_cast<int>(renderTarget);

            if (ImGui::Combo("Render Target", &currentRenderTarget, renderTargetItems, IM_ARRAYSIZE(renderTargetItems)))
            {
                renderTarget = static_cast<RenderTarget>(currentRenderTarget);
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