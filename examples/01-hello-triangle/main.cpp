#include "vgfw.hpp"

int main()
{
    vgfw::hello();

    auto window = vgfw::createWindow({.Width = 800, .Height = 600, .Title = "01-hello-triangle"});

    vgfw::GraphicsContext gc {};
    gc.Init(window);

    vgfw::RenderContext rc {};

    while (!window->ShouldClose())
    {
        if (!window->OnTick())
        {
            break;
        }

        glClearColor(0.2, 0.2, 0.2, 1.0);
        glClear(GL_COLOR_BUFFER_BIT);

        gc.SwapBuffers();
    }

    gc.Shutdown();

    return 0;
}