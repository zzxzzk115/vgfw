#include "vgfw.hpp"

int main()
{
    vgfw::hello();

    auto window = vgfw::createWindow({.Width = 800, .Height = 600, .Title = "01-hello-triangle"});

    while (!window->ShouldClose())
    {
        window->OnTick();
    }

    return 0;
}