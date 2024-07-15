#define VGFW_IMPLEMENTATION
#include "vgfw.hpp"

int main()
{
    // Init VGFW
    if (!vgfw::init())
    {
        std::cerr << "Failed to initialize VGFW" << std::endl;
        return -1;
    }

    // Create a window instance
    auto window = vgfw::window::create({.title = "00-empty-window"});

    // Init renderer
    vgfw::renderer::init({.window = window});

    // Get graphics & render context
    auto& rc = vgfw::renderer::getRenderContext();

    // Main loop
    while (!window->shouldClose())
    {
        window->onTick();

        vgfw::renderer::beginFrame();

        // Render
        rc.beginRendering({.extent = {.width = window->getWidth(), .height = window->getHeight()}},
                          glm::vec4 {0.2f, 0.3f, 0.3f, 1.0f});

        vgfw::renderer::endFrame();

        vgfw::renderer::present();
    }

    // Cleanup
    vgfw::shutdown();

    return 0;
}