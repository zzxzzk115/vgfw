#define VGFW_IMPLEMENTATION
#include "vgfw.hpp"

#include "render_target.hpp"

#include "uniforms/camera_uniform.hpp"
#include "uniforms/light_uniform.hpp"

#include "pass_resource/scene_color_data.hpp"

#include "passes/deferred_lighting_pass.hpp"
#include "passes/final_composition_pass.hpp"
#include "passes/gbuffer_pass.hpp"
#include "passes/tonemapping_pass.hpp"

int main()
{
    // Init VGFW
    if (!vgfw::init())
    {
        std::cerr << "Failed to initialize VGFW" << std::endl;
        return -1;
    }

    // Create a window instance
    auto window = vgfw::window::create({.title = "06-deferred-framegraph"});

    // Init renderer
    vgfw::renderer::init({.window = window});

    // Get render context
    auto& rc = vgfw::renderer::getRenderContext();

    // Create transient resources
    vgfw::renderer::framegraph::TransientResources transientResources(rc);

    // Load model
    vgfw::resource::Model sponza {};
    if (!vgfw::io::load("assets/models/Sponza/glTF/Sponza.gltf", sponza, rc))
    {
        return -1;
    }

    DirectionalLight light {};

    // Camera properties
    Camera camera {};
    camera.data.position = {-1150, 200, -45};
    camera.yaw           = 90.0f;
    camera.updateData(window);

    // Create a texture sampler
    auto sampler = rc.createSampler({.maxAnisotropy = 8});

    vgfw::time::TimePoint lastTime = vgfw::time::Clock::now();

    // Define render passes
    GBufferPass          gBufferPass(rc);
    DeferredLightingPass deferredLightingPass(rc);
    TonemappingPass      tonemappingPass(rc);
    FinalCompositionPass finalCompositionPass(rc);

    // Define render target
    RenderTarget renderTarget = RenderTarget::eFinal;

    // Main loop
    while (!window->shouldClose())
    {
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

        // GBuffer pass
        gBufferPass.addToGraph(
            fg, blackboard, {.width = window->getWidth(), .height = window->getHeight()}, sponza.meshPrimitives);

        // Deferred Lighting pass
        auto& sceneColor = blackboard.add<SceneColorData>();
        sceneColor.hdr   = deferredLightingPass.addToGraph(fg, blackboard);

        // Tone-mapping pass
        sceneColor.ldr = tonemappingPass.addToGraph(fg, sceneColor.hdr);

        // Final composition pass
        finalCompositionPass.compose(fg, blackboard, renderTarget);

        fg.compile();

        vgfw::renderer::beginFrame();

        fg.execute(&rc, &transientResources);

#ifndef NDEBUG
        // Built in graphviz writer.
        std::ofstream {"DebugFrameGraph.dot"} << fg;
#endif

        transientResources.update(dt);

        ImGui::Begin("Deferred (Naive) with FrameGraph");
        ImGui::SliderFloat("Camera FOV", &camera.fov, 1.0f, 179.0f);
        ImGui::Text("Press CAPSLOCK to toggle the camera (W/A/S/D/Q/E + Mouse)");

        const char* comboItems[] = {
            "Final", "GPosition", "GNormal", "GAlbedo", "GEmissive", "GMetallicRoughnessAO", "SceneColorHDR"};

        int currentItem = static_cast<int>(renderTarget);

        if (ImGui::Combo("Render Target", &currentItem, comboItems, IM_ARRAYSIZE(comboItems)))
        {
            renderTarget = static_cast<RenderTarget>(currentItem);
        }
        ImGui::End();

        vgfw::renderer::endFrame();

        vgfw::renderer::present();
    }

    // Cleanup
    vgfw::shutdown();

    return 0;
}