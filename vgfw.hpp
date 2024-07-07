/**
 * @file vgfw.hpp
 * @author Kexuan Zhang (zzxzzk115@gmail.com)
 * @brief
 * @version 0.1.0
 * @date 2024-07-07
 *
 * @copyright Copyright (c) 2024
 *
 */

#pragma once

// Currently, we only support Windows, Linux and macOS.

#define VGFW_PLATFORM_DARWIN 0
#define VGFW_PLATFORM_LINUX 0
#define VGFW_PLATFORM_WINDOWS 0

#if defined(__APPLE__) && defined(__MACH__)
#undef VGFW_PLATFORM_DARWIN
#define VGFW_PLATFORM_DARWIN 1
#elif defined(__linux__)
#undef VGFW_PLATFORM_LINUX
#define VGFW_PLATFORM_LINUX 1
#elif defined(_WIN32) || defined(_WIN64)
#undef VGFW_PLATFORM_WINDOWS
#define VGFW_PLATFORM_WINDOWS 1
#else
#error "Unsupported Platform"
#endif

#include <cstdint>
#include <iostream>
#include <memory>

#include <GLFW/glfw3.h>
#if VGFW_PLATFORM_LINUX
#define GLFW_EXPOSE_NATIVE_X11
#elif VGFW_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#elif VGFW_PLATFORM_DARWIN
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>

namespace vgfw
{
    void hello() { std::cout << "hello!" << std::endl; }

    struct WindowInitInfo
    {
        uint32_t    Width  = 1024;
        uint32_t    Height = 768;
        std::string Title  = "VGFW Window";
        bool        VSync  = true;
    };

    enum class WindowType
    {
        GLFW = 0,
    };

    class Window
    {
    public:
        virtual ~Window() = default;

        /**
         * @brief Get the window type
         *
         * @return WindowType
         */
        virtual WindowType GetType() = 0;

        /**
         * @brief Initialize the window
         *
         * @param windowInitInfo
         */
        virtual bool Init(const WindowInitInfo& windowInitInfo) = 0;

        /**
         * @brief Called each frame
         *
         * @return true
         * @return false
         */
        virtual bool OnTick() = 0;

        /**
         * @brief Get the width of window
         *
         * @return uint32_t
         */
        virtual uint32_t GetWidth() const = 0;

        /**
         * @brief Get the height of window
         *
         * @return uint32_t
         */
        virtual uint32_t GetHeight() const = 0;

        virtual bool ShouldClose() const = 0;
        virtual bool IsMinimized() const = 0;

        virtual void MakeCurrentContext() = 0;
        virtual void SwapBuffers()        = 0;

        virtual void SetHideCursor(bool hide) = 0;

        virtual void* GetPlatformWindow() const = 0;
        virtual void* GetNativeWindow() const   = 0;

    protected:
        /**
         * @brief Cleanup resources and shutdown the window.
         *
         */
        virtual void Shutdown() = 0;
    };

    class GLFWWindow final : public Window
    {
    public:
        ~GLFWWindow() { Shutdown(); }

        virtual WindowType GetType() override { return WindowType::GLFW; }

        virtual bool Init(const WindowInitInfo& initInfo) override;

        virtual bool OnTick() override;

        virtual uint32_t GetWidth() const override { return m_Data.Width; }

        virtual uint32_t GetHeight() const override { return m_Data.Height; }

        virtual bool ShouldClose() const override;
        virtual bool IsMinimized() const override;

        virtual void MakeCurrentContext() override;
        virtual void SwapBuffers() override;

        virtual void SetHideCursor(bool hide) override;

        virtual void* GetPlatformWindow() const override { return m_Window; }

        virtual void* GetNativeWindow() const override
        {
#if VGFW_PLATFORM_LINUX
            return (void*)(uintptr_t)glfwGetX11Window(m_Window);
#elif VGFW_PLATFORM_WINDOWS
            return glfwGetWin32Window(m_Window);
#elif VGFW_PLATFORM_DARWIN
            return glfwGetCocoaWindow(m_Window);
#endif
        }

    protected:
        virtual void Shutdown() override;

    private:
        GLFWwindow* m_Window {nullptr};

        struct WindowData
        {
            std::string  Title;
            unsigned int Width {0}, Height {0};
            GLFWWindow*  WindowSys {nullptr};
            bool         IsMinimized {false};
        };

        WindowData m_Data;
    };

    bool GLFWWindow::Init(const WindowInitInfo& initInfo)
    {
        if (!glfwInit())
        {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        m_Window = glfwCreateWindow(initInfo.Width, initInfo.Height, initInfo.Title.c_str(), nullptr, nullptr);
        if (!m_Window)
        {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }

        m_Data.Title     = initInfo.Title;
        m_Data.Width     = initInfo.Width;
        m_Data.Height    = initInfo.Height;
        m_Data.WindowSys = this;

        return true;
    }

    void GLFWWindow::Shutdown()
    {
        if (m_Window)
        {
            glfwDestroyWindow(m_Window);
            m_Window = nullptr;
        }

        glfwTerminate();
    }

    bool GLFWWindow::OnTick()
    {
        if (m_Window)
        {
            glfwPollEvents();
            return true;
        }

        return false;
    }

    bool GLFWWindow::ShouldClose() const { return m_Window && glfwWindowShouldClose(m_Window); }

    bool GLFWWindow::IsMinimized() const
    {
        if (m_Window)
        {
            int width, height;
            glfwGetWindowSize(m_Window, &width, &height);
            return width == 0 || height == 0;
        }

        return false;
    }

    void GLFWWindow::MakeCurrentContext()
    {
        if (m_Window)
        {
            glfwMakeContextCurrent(m_Window);
        }
    }

    void GLFWWindow::SwapBuffers()
    {
        if (m_Window)
        {
            glfwSwapBuffers(m_Window);
        }
    }

    void GLFWWindow::SetHideCursor(bool hide)
    {
        if (m_Window)
        {
            glfwSetInputMode(m_Window, GLFW_CURSOR, hide ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);
        }
    }

    std::shared_ptr<Window> createWindow(const WindowInitInfo& windowInitInfo, WindowType type = WindowType::GLFW)
    {
        std::shared_ptr<Window> window = nullptr;

        switch (type)
        {
            case WindowType::GLFW:
                window = std::make_shared<GLFWWindow>();
                break;
        }

        if (!window || !window->Init(windowInitInfo))
        {
            return nullptr;
        }

        return window;
    }
} // namespace vgfw
