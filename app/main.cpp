#define VGFW_IMPLEMENTATION
#include "vgfw.hpp"

#include "render_target.hpp"

#include "uniforms/camera_uniform.hpp"
#include "uniforms/light_uniform.hpp"

#include "pass_resource/scene_color_data.hpp"

#include "passes/final_composition_pass.hpp"
#include "passes/forward_lighting_pass.hpp"
#include "passes/reflective_shadow_map_pass.hpp"
#include "passes/tonemapping_pass.hpp"

#include "lpv_config.hpp"

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
    light.direction = {-0.551486731f, -0.827472687f, 0.105599940f};

    // Camera properties
    Camera camera {};
    camera.data.position = {35, 7, -1.5};
    camera.yaw           = -90.0f;
    camera.updateData(window);

    // Create a texture sampler
    auto sampler = rc.createSampler({.maxAnisotropy = 8});

    vgfw::time::TimePoint lastTime = vgfw::time::Clock::now();

    // Define render passes
    ReflectiveShadowMapPass rsmPass(rc);
    ForwardLightingPass     forwardLightingPass(rc);
    TonemappingPass         tonemappingPass(rc);
    FinalCompositionPass    finalCompositionPass(rc);

    // Define render target
    RenderTarget renderTarget = RenderTarget::eFinal;

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

        // Forward Lighting pass
        auto& sceneColor = blackboard.add<SceneColorData>();
        sceneColor.hdr   = forwardLightingPass.addToGraph(fg,
                                                        blackboard,
                                                          {.width = window->getWidth(), .height = window->getHeight()},
                                                        camera,
                                                        sponza.meshPrimitives);

        // Tone-mapping pass
        sceneColor.ldr = tonemappingPass.addToGraph(fg, sceneColor.hdr);

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

            const char* comboItems[] = {"Final", "SceneColorHDR", "RSMPosition", "RSMNormal", "RSMFlux"};

            int currentItem = static_cast<int>(renderTarget);

            if (ImGui::Combo("Render Target", &currentItem, comboItems, IM_ARRAYSIZE(comboItems)))
            {
                renderTarget = static_cast<RenderTarget>(currentItem);
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