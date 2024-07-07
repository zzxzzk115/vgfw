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

#if VGFW_PLATFORM_WINDOWS
#define NOMINMAX
#endif

#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <set>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#if VGFW_PLATFORM_LINUX
#define GLFW_EXPOSE_NATIVE_X11
#elif VGFW_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#elif VGFW_PLATFORM_DARWIN
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>

#include <vulkan/vulkan.hpp>
#if VGFW_PLATFORM_WINDOWS
#include <vulkan/vulkan_win32.h>
#elif VGFW_PLATFORM_LINUX
#include <vulkan/vulkan_xcb.h>
#elif VGFW_PLATFORM_DARWIN
#include <vulkan/vulkan_macos.h>
#endif

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
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // TODO: After implementing swapchain recreation, set it to true.

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

    bool createWindow(const WindowInitInfo&    windowInitInfo,
                      std::shared_ptr<Window>& window,
                      WindowType               type = WindowType::GLFW)
    {
        assert(window == nullptr);

        switch (type)
        {
            case WindowType::GLFW:
                window = std::make_shared<GLFWWindow>();
                break;
        }

        return window && window->Init(windowInitInfo);
    }

    const std::vector<const char*> g_validationLayers = {"VK_LAYER_KHRONOS_validation"};
    const std::vector<const char*> g_deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    namespace vkutils
    {
        VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/,
                                                     VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
                                                     const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                     void* /*pUserData*/)
        {
            std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

            return VK_FALSE;
        }

        struct QueueFamilyIndices
        {
            std::optional<uint32_t> GraphicsFamily;

            bool IsComplete() const { return GraphicsFamily.has_value(); }
        };

        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device)
        {
            QueueFamilyIndices indices;
            uint32_t           queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

            int i = 0;
            for (const auto& queueFamily : queueFamilies)
            {
                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
                {
                    indices.GraphicsFamily = i;
                }

                i++;
            }

            return indices;
        }

        bool checkDeviceExtensionSupport(vk::PhysicalDevice device, const std::vector<const char*>& requiredExtensions)
        {
            std::vector<vk::ExtensionProperties> availableExtensions = device.enumerateDeviceExtensionProperties();

            std::set<std::string> requiredExtensionsSet(requiredExtensions.begin(), requiredExtensions.end());

            for (const auto& extension : availableExtensions)
            {
                requiredExtensionsSet.erase(extension.extensionName);
            }

            return requiredExtensionsSet.empty();
        }

        bool isDeviceSuitable(VkPhysicalDevice device)
        {
            QueueFamilyIndices indices = findQueueFamilies(device);
            return indices.IsComplete() && checkDeviceExtensionSupport(device, g_deviceExtensions);
        }

        struct SwapchainSupportDetails
        {
            vk::SurfaceCapabilitiesKHR        Capabilities;
            std::vector<vk::SurfaceFormatKHR> Formats;
            std::vector<vk::PresentModeKHR>   PresentModes;
        };

        SwapchainSupportDetails querySwapchainSupport(vk::PhysicalDevice physicalDevice, vk::SurfaceKHR surface)
        {
            SwapchainSupportDetails details;

            details.Capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
            details.Formats      = physicalDevice.getSurfaceFormatsKHR(surface);
            details.PresentModes = physicalDevice.getSurfacePresentModesKHR(surface);

            return details;
        }

        vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
        {
            if (availableFormats.size() == 1 && availableFormats[0].format == vk::Format::eUndefined)
            {
                return {vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};
            }

            for (const auto& format : availableFormats)
            {
                if (format.format == vk::Format::eB8G8R8A8Unorm &&
                    format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
                {
                    return format;
                }
            }

            return availableFormats[0];
        }

        vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
        {
            for (const auto& mode : availablePresentModes)
            {
                if (mode == vk::PresentModeKHR::eMailbox)
                {
                    return mode;
                }
            }
            return vk::PresentModeKHR::eFifo;
        }

        vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height)
        {
            if (capabilities.currentExtent.width != UINT32_MAX)
            {
                return capabilities.currentExtent;
            }
            else
            {
                vk::Extent2D actualExtent = {width, height};

                actualExtent.width  = std::max(capabilities.minImageExtent.width,
                                              std::min(capabilities.maxImageExtent.width, actualExtent.width));
                actualExtent.height = std::max(capabilities.minImageExtent.height,
                                               std::min(capabilities.maxImageExtent.height, actualExtent.height));

                return actualExtent;
            }
        }
    } // namespace vkutils

    class VulkanRHI
    {
    public:
        bool Init(const std::shared_ptr<Window>& window, bool enableValidationLayers);
        void Shutdown();

        vk::Instance                      GetInstance() const { return m_Instance; }
        vk::SurfaceKHR                    GetSurface() const { return m_Surface; }
        vk::PhysicalDevice                GetPhysicalDevice() const { return m_PhysicalDevice; }
        uint32_t                          GetGraphicsQueueFamilyIndex() const { return m_GraphicsQueueFamilyIndex; }
        uint32_t                          GetPresentQueueFamilyIndex() const { return m_PresentQueueFamilyIndex; }
        vk::SwapchainKHR                  GetSwapchain() const { return m_Swapchain; }
        const std::vector<vk::Image>&     GetSwapchainImages() const { return m_SwapchainImages; }
        const std::vector<vk::ImageView>& GetSwapchainImageViews() const { return m_SwapchainImageViews; }

    private:
        bool CreateInstance(bool enableValidationLayers);
        bool CreateSurface(const std::shared_ptr<Window>& window);
        bool SetupDebugCallback();
        void DestroyDebugCallback();
        bool PickPhysicalDevice();
        bool FindQueueFamilies();
        bool CreateLogicalDevice();
        bool CreateSwapchain();
        bool CreateImageViews();

        std::shared_ptr<Window>    m_Window {nullptr};
        vk::Instance               m_Instance {VK_NULL_HANDLE};
        vk::SurfaceKHR             m_Surface {VK_NULL_HANDLE};
        VkDebugUtilsMessengerEXT   m_DebugMessenger {VK_NULL_HANDLE};
        vk::PhysicalDevice         m_PhysicalDevice {VK_NULL_HANDLE};
        uint32_t                   m_GraphicsQueueFamilyIndex {UINT32_MAX};
        uint32_t                   m_PresentQueueFamilyIndex {UINT32_MAX};
        vk::Device                 m_Device {VK_NULL_HANDLE};
        vk::Queue                  m_GraphicsQueue {VK_NULL_HANDLE};
        vk::Queue                  m_PresentQueue {VK_NULL_HANDLE};
        vk::SwapchainKHR           m_Swapchain {VK_NULL_HANDLE};
        std::vector<vk::Image>     m_SwapchainImages;
        std::vector<vk::ImageView> m_SwapchainImageViews;
    };

    bool VulkanRHI::Init(const std::shared_ptr<Window>& window, bool enableValidationLayers)
    {
        m_Window = window;

        if (!CreateInstance(enableValidationLayers))
        {
            return false;
        }

        if (enableValidationLayers && !SetupDebugCallback())
        {
            return false;
        }

        if (!CreateSurface(window))
        {
            return false;
        }

        if (!PickPhysicalDevice())
        {
            return false;
        }

        if (!FindQueueFamilies())
        {
            return false;
        }

        if (!CreateLogicalDevice())
        {
            return false;
        }

        if (!CreateSwapchain())
        {
            return false;
        }

        if (!CreateImageViews())
        {
            return false;
        }

        return true;
    }

    void VulkanRHI::Shutdown()
    {
        if (m_Device)
        {
            if (m_Swapchain)
            {
                for (auto& swapchainImageView : m_SwapchainImageViews)
                {
                    m_Device.destroyImageView(swapchainImageView);
                }
                m_SwapchainImageViews.clear();

                m_Device.destroySwapchainKHR(m_Swapchain);
                m_Swapchain = nullptr;
            }

            m_Device.destroy();
            m_Device = nullptr;
        }

        if (m_DebugMessenger != VK_NULL_HANDLE)
        {
            DestroyDebugCallback();
        }

        if (m_Surface)
        {
            m_Instance.destroySurfaceKHR(m_Surface);
            m_Surface = nullptr;
        }

        if (m_Instance)
        {
            m_Instance.destroy();
            m_Instance = nullptr;
        }
    }

    bool VulkanRHI::CreateInstance(bool enableValidationLayers)
    {
        vk::ApplicationInfo appInfo {};
        appInfo.pApplicationName   = "VGFW Application";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName        = "VGFW";
        appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion         = VK_API_VERSION_1_3;

        std::vector<const char*> extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
#if VGFW_PLATFORM_WINDOWS
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#elif VGFW_PLATFORM_LINUX
            VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
#elif VGFW_PLATFORM_DARWIN
            VK_EXT_METAL_SURFACE_EXTENSION_NAME,
#endif
        };

        // enable debug utils if enable validation layers
        if (enableValidationLayers)
        {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        vk::InstanceCreateInfo createInfo {};
        createInfo.pApplicationInfo        = &appInfo;
        createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        // Enable validation layers if requested and available
        if (enableValidationLayers)
        {
            createInfo.enabledLayerCount   = static_cast<uint32_t>(g_validationLayers.size());
            createInfo.ppEnabledLayerNames = g_validationLayers.data();
        }
        else
        {
            createInfo.enabledLayerCount = 0;
        }

        vk::Result result = vk::createInstance(&createInfo, nullptr, &m_Instance);
        if (result != vk::Result::eSuccess)
        {
            std::cerr << "Failed to create Vulkan instance: " << vk::to_string(result) << std::endl;
            return false;
        }

        return true;
    }

    bool VulkanRHI::CreateSurface(const std::shared_ptr<Window>& window)
    {
        if (window->GetType() != WindowType::GLFW)
        {
            std::cerr << "Unsupported window type for Vulkan surface creation" << std::endl;
            return false;
        }

        GLFWwindow*  glfwWindow = static_cast<GLFWwindow*>(window->GetPlatformWindow());
        VkSurfaceKHR surface;

        VkResult result = glfwCreateWindowSurface(m_Instance, glfwWindow, nullptr, &surface);
        if (result != VK_SUCCESS)
        {
            std::cerr << "Failed to create window surface: " << result << std::endl;
            return false;
        }
        m_Surface = surface;

        return true;
    }

    bool VulkanRHI::SetupDebugCallback()
    {
        vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose |
                                                            vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
                                                            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
        vk::DebugUtilsMessageTypeFlagsEXT     messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
                                                           vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
                                                           vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance);

        vk::DebugUtilsMessengerCreateInfoEXT createInfo(
            {}, severityFlags, messageTypeFlags, vkutils::debugCallback, nullptr);

        auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            m_Instance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));
        if (!func)
        {
            std::cerr << "Failed to get vkCreateDebugUtilsMessengerEXT function pointer." << std::endl;
            return false;
        }

        VkResult result = func(m_Instance,
                               reinterpret_cast<const VkDebugUtilsMessengerCreateInfoEXT*>(&createInfo),
                               nullptr,
                               &m_DebugMessenger);
        if (result != VK_SUCCESS)
        {
            std::cerr << "Failed to set up debug messenger: " << result << std::endl;
            return false;
        }

        return true;
    }

    void VulkanRHI::DestroyDebugCallback()
    {
        // Get function pointer for debug messenger destruction
        auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            m_Instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT"));
        if (func)
        {
            func(m_Instance, m_DebugMessenger, nullptr);
            m_DebugMessenger = VK_NULL_HANDLE;
        }
    }

    bool VulkanRHI::PickPhysicalDevice()
    {
        auto physicalDevices = m_Instance.enumeratePhysicalDevices();
        if (physicalDevices.empty())
        {
            std::cerr << "Failed to find GPUs with Vulkan support!" << std::endl;
            return false;
        }

        for (const auto& device : physicalDevices)
        {
            if (vkutils::isDeviceSuitable(device))
            {
                m_PhysicalDevice = device;
                return true;
            }
        }

        std::cerr << "Failed to find a suitable GPU!" << std::endl;
        return false;
    }

    bool VulkanRHI::FindQueueFamilies()
    {
        auto queueFamilies = m_PhysicalDevice.getQueueFamilyProperties();
        for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilies.size()); ++i)
        {
            const auto& queueFamily = queueFamilies[i];

            if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
            {
                m_GraphicsQueueFamilyIndex = i;
            }

            vk::Bool32 presentSupport = m_PhysicalDevice.getSurfaceSupportKHR(i, m_Surface);
            if (presentSupport)
            {
                m_PresentQueueFamilyIndex = i;
            }

            if (m_GraphicsQueueFamilyIndex != UINT32_MAX && m_PresentQueueFamilyIndex != UINT32_MAX)
            {
                return true;
            }
        }

        std::cerr << "Failed to find suitable queue families!" << std::endl;
        return false;
    }

    bool VulkanRHI::CreateLogicalDevice()
    {
        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {m_GraphicsQueueFamilyIndex, m_PresentQueueFamilyIndex};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            vk::DeviceQueueCreateInfo queueCreateInfo({}, queueFamily, 1, &queuePriority);
            queueCreateInfos.push_back(queueCreateInfo);
        }

        vk::PhysicalDeviceFeatures deviceFeatures {}; // TODO: add features, expose API

        vk::DeviceCreateInfo createInfo {};
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos    = queueCreateInfos.data();
        createInfo.pEnabledFeatures     = &deviceFeatures;

        // TODO: add possible layers, expose API
        createInfo.enabledExtensionCount   = static_cast<uint32_t>(g_deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = g_deviceExtensions.data();
        createInfo.enabledLayerCount       = 0;

        vk::Result result = m_PhysicalDevice.createDevice(&createInfo, nullptr, &m_Device);
        if (result != vk::Result::eSuccess)
        {
            std::cerr << "Failed to create logical device: " << vk::to_string(result) << std::endl;
            return false;
        }

        // Retrieve queue handles
        m_GraphicsQueue = m_Device.getQueue(m_GraphicsQueueFamilyIndex, 0);
        m_PresentQueue  = m_Device.getQueue(m_PresentQueueFamilyIndex, 0);

        return true;
    }

    bool VulkanRHI::CreateSwapchain()
    {
        auto swapchainSupport = vkutils::querySwapchainSupport(m_PhysicalDevice, m_Surface);

        vk::SurfaceFormatKHR surfaceFormat = vkutils::chooseSwapSurfaceFormat(swapchainSupport.Formats);
        vk::PresentModeKHR   presentMode   = vkutils::chooseSwapPresentMode(swapchainSupport.PresentModes);
        vk::Extent2D         extent =
            vkutils::chooseSwapExtent(swapchainSupport.Capabilities, m_Window->GetWidth(), m_Window->GetHeight());

        uint32_t imageCount = swapchainSupport.Capabilities.minImageCount + 1;
        if (swapchainSupport.Capabilities.maxImageCount > 0 && imageCount > swapchainSupport.Capabilities.maxImageCount)
        {
            imageCount = swapchainSupport.Capabilities.maxImageCount;
        }

        vk::SwapchainCreateInfoKHR createInfo {};
        createInfo.surface          = m_Surface;
        createInfo.minImageCount    = imageCount;
        createInfo.imageFormat      = surfaceFormat.format;
        createInfo.imageColorSpace  = surfaceFormat.colorSpace;
        createInfo.imageExtent      = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment;

        uint32_t queueFamilyIndices[] = {m_GraphicsQueueFamilyIndex, m_PresentQueueFamilyIndex};
        if (m_GraphicsQueueFamilyIndex != m_PresentQueueFamilyIndex)
        {
            createInfo.imageSharingMode      = vk::SharingMode::eConcurrent;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices   = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode      = vk::SharingMode::eExclusive;
            createInfo.queueFamilyIndexCount = 0;       // Optional
            createInfo.pQueueFamilyIndices   = nullptr; // Optional
        }

        createInfo.preTransform   = swapchainSupport.Capabilities.currentTransform;
        createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
        createInfo.presentMode    = presentMode;
        createInfo.clipped        = VK_TRUE;
        createInfo.oldSwapchain   = VK_NULL_HANDLE;

        vk::Result result = m_Device.createSwapchainKHR(&createInfo, nullptr, &m_Swapchain);
        if (result != vk::Result::eSuccess)
        {
            std::cerr << "Failed to create swap chain: " << vk::to_string(result) << std::endl;
            return false;
        }

        return true;
    }

    bool VulkanRHI::CreateImageViews()
    {
        m_SwapchainImages = m_Device.getSwapchainImagesKHR(m_Swapchain);

        m_SwapchainImageViews.resize(m_SwapchainImages.size());
        for (size_t i = 0; i < m_SwapchainImages.size(); ++i)
        {
            vk::ImageViewCreateInfo createInfo {};
            createInfo.image                           = m_SwapchainImages[i];
            createInfo.viewType                        = vk::ImageViewType::e2D;
            createInfo.format                          = vk::Format::eB8G8R8A8Unorm;
            createInfo.components.r                    = vk::ComponentSwizzle::eIdentity;
            createInfo.components.g                    = vk::ComponentSwizzle::eIdentity;
            createInfo.components.b                    = vk::ComponentSwizzle::eIdentity;
            createInfo.components.a                    = vk::ComponentSwizzle::eIdentity;
            createInfo.subresourceRange.aspectMask     = vk::ImageAspectFlagBits::eColor;
            createInfo.subresourceRange.baseMipLevel   = 0;
            createInfo.subresourceRange.levelCount     = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount     = 1;

            vk::Result result = m_Device.createImageView(&createInfo, nullptr, &m_SwapchainImageViews[i]);
            if (result != vk::Result::eSuccess)
            {
                std::cerr << "Failed to create image views: " << vk::to_string(result) << std::endl;
                return false;
            }
        }

        return true;
    }
} // namespace vgfw
