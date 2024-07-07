#include "vgfw.hpp"

int main()
{
    vgfw::hello();

    std::shared_ptr<vgfw::Window> window = nullptr;

    if (!vgfw::createWindow({.Width = 800, .Height = 600, .Title = "01-hello-triangle"}, window))
    {
        return -1;
    }

    vgfw::VulkanRHI rhi {};

#ifndef N_DEBUG
    bool enableValidationLayers = true;
#else
    bool enableValidationLayers = false;
#endif

    if (!rhi.Init(window, enableValidationLayers))
    {
        return -1;
    }

    while (!window->ShouldClose())
    {
        if (!window->OnTick())
        {
            break;
        }
    }

    rhi.Shutdown();

    return 0;
}