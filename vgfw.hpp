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

#include <stdexcept>
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

#define VGFW_RENDER_API_OPENGL_MIN_MAJOR 3
#define VGFW_RENDER_API_OPENGL_MIN_MINOR 3

// force OpenGL 4.6 on Windows
#if VGFW_PLATFORM_WINDOWS
#undef VGFW_RENDER_API_OPENGL_MIN_MAJOR
#undef VGFW_RENDER_API_OPENGL_MIN_MINOR
#define VGFW_RENDER_API_OPENGL_MIN_MAJOR 4
#define VGFW_RENDER_API_OPENGL_MIN_MINOR 6
#endif

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <variant>

#include <glad/glad.h>

#include <GLFW/glfw3.h>
#if VGFW_PLATFORM_LINUX
#define GLFW_EXPOSE_NATIVE_X11
#elif VGFW_PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#elif VGFW_PLATFORM_DARWIN
#define GLFW_EXPOSE_NATIVE_COCOA
#endif
#include <GLFW/glfw3native.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace vgfw
{
    void hello() { std::cout << "hello!" << std::endl; }

    namespace utils
    {
        template<typename T, typename... Rest>
        static inline void hashCombine(std::size_t& seed, const T& v, const Rest&... rest)
        {
            // https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
            seed ^= std::hash<T> {}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            (hashCombine(seed, rest), ...);
        }
    } // namespace utils

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
         */
        virtual void OnTick() = 0;

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

        virtual void OnTick() override;

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

        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, VGFW_RENDER_API_OPENGL_MIN_MAJOR);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, VGFW_RENDER_API_OPENGL_MIN_MINOR);

        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

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

    void GLFWWindow::OnTick() { glfwPollEvents(); }

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
            throw std::runtime_error("Failed to initialize window!");
        }

        return window;
    }

    class GraphicsContext
    {
    public:
        GraphicsContext() = default;

        void Init(const std::shared_ptr<Window>& window);

        void        SwapBuffers();
        static void SetVSync(bool vsyncEnabled);

        bool IsSupportDSA() const { return m_SupportDSA; }

    private:
        static int LoadGL();
        static int GetMinMajor();
        static int GetMinMinor();

    protected:
        bool                    m_SupportDSA {false};
        std::shared_ptr<Window> m_Window {nullptr};
    };

    void GraphicsContext::Init(const std::shared_ptr<Window>& window)
    {
        m_Window = window;

        window->MakeCurrentContext();
        int version = LoadGL();

        assert(version);
        if (version)
        {
            int minMajor = GetMinMajor();
            int minMinor = GetMinMinor();
            std::cout << "[OpenGLContext] Loaded " << GLVersion.major << "." << GLVersion.minor << std::endl;

            std::stringstream ss;
            ss << "    Vendor:       " << glGetString(GL_VENDOR) << std::endl;
            ss << "    Version:      " << glGetString(GL_VERSION) << std::endl;
            ss << "    Renderer:     " << glGetString(GL_RENDERER) << std::endl;
            ss << "    GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
            std::cout << "[OpenGLContext] GL Context Info:\n" << ss.str() << std::endl;

            assert(GLVersion.major > minMajor || (GLVersion.major == minMajor && GLVersion.minor >= minMinor));
        }

        m_SupportDSA = GLAD_GL_VERSION_4_5 || GLAD_GL_VERSION_4_6;
    }

    void GraphicsContext::SwapBuffers() { m_Window->SwapBuffers(); }

    void GraphicsContext::SetVSync(bool vsyncEnabled) { glfwSwapInterval(vsyncEnabled); }

    int GraphicsContext::LoadGL() { return gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)); }

    int GraphicsContext::GetMinMajor() { return VGFW_RENDER_API_OPENGL_MIN_MAJOR; }

    int GraphicsContext::GetMinMinor() { return VGFW_RENDER_API_OPENGL_MIN_MINOR; }

    class Buffer
    {
        friend class RenderContext;

    public:
        Buffer()              = default;
        Buffer(const Buffer&) = delete;
        Buffer(Buffer&&) noexcept;
        virtual ~Buffer();

        Buffer& operator=(const Buffer&) = delete;
        Buffer& operator=(Buffer&&) noexcept;

        explicit operator bool() const;

        GLsizeiptr GetSize() const;
        bool       IsMapped() const;

    protected:
        Buffer(GLuint id, GLsizeiptr size);

        explicit operator GLuint() const;

    protected:
        GLuint     m_Id {GL_NONE};
        GLsizeiptr m_Size {0};
        void*      m_MappedMemory {nullptr};
    };

    Buffer::Buffer(Buffer&& other) noexcept :
        m_Id(other.m_Id), m_Size(other.m_Size), m_MappedMemory(other.m_MappedMemory)
    {
        memset(&other, 0, sizeof(Buffer));
    }

    Buffer::~Buffer()
    {
        if (m_Id != GL_NONE)
            std::cerr << "Buffer leak: " << m_Id << std::endl;
    }

    Buffer& Buffer::operator=(Buffer&& rhs) noexcept
    {
        if (this != &rhs)
        {
            memcpy(this, &rhs, sizeof(Buffer));
            memset(&rhs, 0, sizeof(Buffer));
        }
        return *this;
    }

    Buffer::operator bool() const { return m_Id != GL_NONE; }

    GLsizeiptr Buffer::GetSize() const { return m_Size; }
    bool       Buffer::IsMapped() const { return m_MappedMemory != nullptr; }

    Buffer::Buffer(GLuint id, GLsizeiptr size) : m_Id(id), m_Size(size) {}

    Buffer::operator GLuint() const { return m_Id; }

    enum class IndexType
    {
        Unknown = 0,
        UInt8   = 1,
        UInt16  = 2,
        UInt32  = 4
    };

    class IndexBuffer final : public Buffer
    {
        friend class RenderContext;

    public:
        IndexBuffer() = default;

        IndexType  GetIndexType() const;
        GLsizeiptr GetCapacity() const;

    private:
        IndexBuffer(Buffer, IndexType);

    private:
        IndexType m_IndexType {IndexType::Unknown};
    };

    IndexType  IndexBuffer::GetIndexType() const { return m_IndexType; }
    GLsizeiptr IndexBuffer::GetCapacity() const { return m_Size / static_cast<GLsizei>(m_IndexType); }

    IndexBuffer::IndexBuffer(Buffer buffer, IndexType indexType) : Buffer {std::move(buffer)}, m_IndexType {indexType}
    {}

    class VertexBuffer final : public Buffer
    {
        friend class RenderContext;

    public:
        VertexBuffer() = default;

        GLsizei    GetStride() const;
        GLsizeiptr GetCapacity() const;

    private:
        VertexBuffer(Buffer, GLsizei stride);

    private:
        GLsizei m_Stride {0};
    };

    GLsizei    VertexBuffer::GetStride() const { return m_Stride; }
    GLsizeiptr VertexBuffer::GetCapacity() const { return m_Size / m_Stride; }

    VertexBuffer::VertexBuffer(Buffer buffer, GLsizei stride) : Buffer {std::move(buffer)}, m_Stride {stride} {}

    struct VertexAttribute
    {
        enum class Type
        {
            Float = 0,
            Float2,
            Float3,
            Float4,

            Int,
            Int4,

            UByte4_Norm,
        };
        Type    VertType;
        int32_t Offset;
    };

    using VertexAttributes = std::map<int32_t, VertexAttribute>;

    enum class AttributeLocation : int32_t
    {
        Position = 0,
        Normal,
        TexCoords,
        Tangent,
        Bitangent,
    };

    class VertexFormat
    {
    public:
        VertexFormat()                        = delete;
        VertexFormat(const VertexFormat&)     = delete;
        VertexFormat(VertexFormat&&) noexcept = default;
        ~VertexFormat()                       = default;

        VertexFormat& operator=(const VertexFormat&)     = delete;
        VertexFormat& operator=(VertexFormat&&) noexcept = delete;

        std::size_t GetHash() const;

        const VertexAttributes& GetAttributes() const;
        bool                    Contains(AttributeLocation) const;
        bool                    Contains(std::initializer_list<AttributeLocation>) const;

        uint32_t GetStride() const;

        class Builder final
        {
        public:
            Builder()                   = default;
            Builder(const Builder&)     = delete;
            Builder(Builder&&) noexcept = delete;
            ~Builder()                  = default;

            Builder& operator=(const Builder&)     = delete;
            Builder& operator=(Builder&&) noexcept = delete;

            Builder& SetAttribute(AttributeLocation, const VertexAttribute&);

            std::shared_ptr<VertexFormat> Build();
            std::shared_ptr<VertexFormat> BuildDefault();

        private:
            VertexAttributes m_Attributes;

            using Cache = std::unordered_map<std::size_t, std::weak_ptr<VertexFormat>>;
            inline static Cache s_Cache;
        };

    private:
        VertexFormat(std::size_t hash, VertexAttributes&&, uint32_t stride);

    private:
        const std::size_t      m_Hash {0u};
        const VertexAttributes m_Attributes;
        const uint32_t         m_Stride {0};
    };

    int32_t getSize(VertexAttribute::Type type)
    {
        switch (type)
        {
            using enum VertexAttribute::Type;
            case Float:
                return sizeof(float);
            case Float2:
                return sizeof(float) * 2;
            case Float3:
                return sizeof(float) * 3;
            case Float4:
                return sizeof(float) * 4;

            case VertexAttribute::Type::Int:
                return sizeof(int32_t);
            case Int4:
                return sizeof(int32_t) * 4;

            case UByte4_Norm:
                return sizeof(uint8_t) * 4;
        }
        return 0;
    }

    std::size_t VertexFormat::GetHash() const { return m_Hash; }

    const VertexAttributes& VertexFormat::GetAttributes() const { return m_Attributes; }
    bool                    VertexFormat::Contains(AttributeLocation location) const
    {
        return m_Attributes.contains(static_cast<int32_t>(location));
    }

    bool VertexFormat::Contains(std::initializer_list<AttributeLocation> locations) const
    {
        return std::all_of(std::cbegin(locations), std::cend(locations), [&](auto location) {
            return std::any_of(m_Attributes.cbegin(),
                               m_Attributes.cend(),
                               [v = static_cast<int32_t>(location)](const auto& p) { return p.first == v; });
        });
    }

    uint32_t VertexFormat::GetStride() const { return m_Stride; }

    VertexFormat::VertexFormat(std::size_t hash, VertexAttributes&& attributes, uint32_t stride) :
        m_Hash {hash}, m_Attributes {attributes}, m_Stride {stride}
    {}

    using Builder = VertexFormat::Builder;

    Builder& Builder::SetAttribute(AttributeLocation location, const VertexAttribute& attribute)
    {
        m_Attributes.insert_or_assign(static_cast<int32_t>(location), attribute);
        return *this;
    }

    std::shared_ptr<VertexFormat> Builder::Build()
    {
        uint32_t    stride {0};
        std::size_t hash {0};
        for (const auto& [location, attribute] : m_Attributes)
        {
            stride += getSize(attribute.VertType);
            utils::hashCombine(hash, location, attribute);
        }

        if (const auto it = s_Cache.find(hash); it != s_Cache.cend())
            if (auto vertexFormat = it->second.lock(); vertexFormat)
                return vertexFormat;

        auto vertexFormat = std::make_shared<VertexFormat>(VertexFormat {hash, std::move(m_Attributes), stride});
        s_Cache.insert_or_assign(hash, vertexFormat);
        return vertexFormat;
    }

    std::shared_ptr<VertexFormat> Builder::BuildDefault()
    {
        m_Attributes.clear();

        SetAttribute(AttributeLocation::Position, {.VertType = VertexAttribute::Type::Float3, .Offset = 0});
        SetAttribute(AttributeLocation::Normal, {.VertType = VertexAttribute::Type::Float3, .Offset = 12});
        SetAttribute(AttributeLocation::TexCoords, {.VertType = VertexAttribute::Type::Float2, .Offset = 24});

        return Build();
    }

    // clang-format off
    struct Offset2D
    {
        int32_t X {0}, Y {0};

        auto operator<=> (const Offset2D&) const = default;
    };

    struct Extent2D
    {
        uint32_t Width {0}, Height {0};

        auto operator<=> (const Extent2D&) const = default;
    };

    struct Rect2D
    {
        Offset2D Offset;
        Extent2D Extent;

        auto operator<=> (const Rect2D&) const = default;
    };
    // clang-format on

    enum class CompareOp : GLenum
    {
        Never          = GL_NEVER,
        Less           = GL_LESS,
        Equal          = GL_EQUAL,
        LessOrEqual    = GL_LEQUAL,
        Greater        = GL_GREATER,
        NotEqual       = GL_NOTEQUAL,
        GreaterOrEqual = GL_GEQUAL,
        Always         = GL_ALWAYS
    };

    using ViewportDesc = Rect2D;

    enum class PixelFormat : GLenum
    {
        Unknown = GL_NONE,

        R8_UNorm = GL_R8,
        R32I     = GL_R32I,

        RGB8_UNorm  = GL_RGB8,
        RGBA8_UNorm = GL_RGBA8,

        RGB8_SNorm  = GL_RGB8_SNORM,
        RGBA8_SNorm = GL_RGBA8_SNORM,

        R16F    = GL_R16F,
        RG16F   = GL_RG16F,
        RGB16F  = GL_RGB16F,
        RGBA16F = GL_RGBA16F,

        RGB32F = GL_RGB32F,

        RGBA32F = GL_RGBA32F,

        RGBA32UI = GL_RGBA32UI,

        Depth16  = GL_DEPTH_COMPONENT16,
        Depth24  = GL_DEPTH_COMPONENT24,
        Depth32F = GL_DEPTH_COMPONENT32F
    };

    enum class TextureType : GLenum
    {
        Texture2D = GL_TEXTURE_2D,
        CubeMap   = GL_TEXTURE_CUBE_MAP
    };

    class Texture
    {
        friend class RenderContext;

    public:
        Texture()               = default;
        Texture(const Texture&) = delete;
        Texture(Texture&&) noexcept;
        ~Texture();

        Texture& operator=(const Texture&) = delete;
        Texture& operator=(Texture&&) noexcept;

        explicit operator bool() const;

        GLenum GetType() const;

        Extent2D    GetExtent() const;
        uint32_t    GetDepth() const;
        uint32_t    GetNumMipLevels() const;
        uint32_t    GetNumLayers() const;
        PixelFormat GetPixelFormat() const;

    private:
        Texture(GLuint id,
                GLenum type,
                PixelFormat,
                Extent2D,
                uint32_t depth,
                uint32_t numMipLevels,
                uint32_t numLayers);

        explicit operator GLuint() const;

    private:
        GLuint m_Id {GL_NONE};
        GLenum m_Type {GL_NONE};

        GLuint m_View {GL_NONE};

        Extent2D m_Extent {0u};
        uint32_t m_Depth {0u};
        uint32_t m_NumMipLevels {1u};
        uint32_t m_NumLayers {0u};

        PixelFormat m_PixelFormat {PixelFormat::Unknown};
    };

    enum class TexelFilter : GLenum
    {
        Nearest = GL_NEAREST,
        Linear  = GL_LINEAR
    };
    enum class MipmapMode : GLenum
    {
        None    = GL_NONE,
        Nearest = GL_NEAREST,
        Linear  = GL_LINEAR
    };

    enum class SamplerAddressMode : GLenum
    {
        Repeat            = GL_REPEAT,
        MirroredRepeat    = GL_MIRRORED_REPEAT,
        ClampToEdge       = GL_CLAMP_TO_EDGE,
        ClampToBorder     = GL_CLAMP_TO_BORDER,
        MirrorClampToEdge = GL_MIRROR_CLAMP_TO_EDGE
    };

    struct SamplerInfo
    {
        TexelFilter MinFilter {TexelFilter::Nearest};
        MipmapMode  MipmapMode {MipmapMode::Linear};
        TexelFilter MagFilter {TexelFilter::Linear};

        SamplerAddressMode AddressModeS {SamplerAddressMode::Repeat};
        SamplerAddressMode AddressModeT {SamplerAddressMode::Repeat};
        SamplerAddressMode AddressModeR {SamplerAddressMode::Repeat};

        float MaxAnisotropy {1.0f};

        std::optional<CompareOp> CompareOperator {};
        glm::vec4                BorderColor {0.0f};
    };

    Texture::Texture(Texture&& other) noexcept :
        m_Id {other.m_Id}, m_Type {other.m_Type}, m_View {other.m_View}, m_Extent {other.m_Extent},
        m_Depth {other.m_Depth}, m_NumMipLevels {other.m_NumMipLevels}, m_NumLayers {other.m_NumLayers},
        m_PixelFormat {other.m_PixelFormat}
    {
        memset(&other, 0, sizeof(Texture));
    }

    Texture::~Texture()
    {
        if (m_Id != GL_NONE)
            std::cerr << "Texture leak: " << m_Id << std::endl;
    }

    Texture& Texture::operator=(Texture&& rhs) noexcept
    {
        if (this != &rhs)
        {
            memcpy(this, &rhs, sizeof(Texture));
            memset(&rhs, 0, sizeof(Texture));
        }
        return *this;
    }

    Texture::operator bool() const { return m_Id != GL_NONE; }

    GLenum Texture::GetType() const { return m_Type; }

    Extent2D    Texture::GetExtent() const { return m_Extent; }
    uint32_t    Texture::GetDepth() const { return m_Depth; }
    uint32_t    Texture::GetNumMipLevels() const { return m_NumMipLevels; }
    uint32_t    Texture::GetNumLayers() const { return m_NumLayers; }
    PixelFormat Texture::GetPixelFormat() const { return m_PixelFormat; }

    Texture::Texture(GLuint      id,
                     GLenum      type,
                     PixelFormat pixelFormat,
                     Extent2D    extent,
                     uint32_t    depth,
                     uint32_t    numMipLevels,
                     uint32_t    numLayers) :
        m_Id {id},
        m_Type {type}, m_Extent {extent}, m_Depth {depth}, m_NumMipLevels {numMipLevels}, m_NumLayers {numLayers},
        m_PixelFormat {pixelFormat}
    {}

    Texture::operator GLuint() const { return m_Id; }

    uint32_t   calcMipLevels(uint32_t size) { return glm::floor(glm::log2(static_cast<float>(size))) + 1u; }
    glm::uvec3 calcMipSize(const glm::uvec3& baseSize, uint32_t level)
    {
        return glm::vec3 {baseSize} * glm::pow(0.5f, static_cast<float>(level));
    }

    const char* toString(PixelFormat pixelFormat)
    {
        switch (pixelFormat)
        {
            using enum PixelFormat;

            case R8_UNorm:
                return "R8_Unorm";

            case RGB8_UNorm:
                return "RGB8_UNorm";
            case RGBA8_UNorm:
                return "RGBA8_UNorm";

            case RGB8_SNorm:
                return "RGB8_SNorm";
            case RGBA8_SNorm:
                return "RGBA8_SNorm";

            case R16F:
                return "R16F";
            case RG16F:
                return "RG16F";
            case RGB16F:
                return "RGB16F";
            case RGBA16F:
                return "RGBA16F";

            case RGB32F:
                return "RGB32F";
            case RGBA32F:
                return "RGBA32F";

            case R32I:
                return "R32I";
            case RGBA32UI:
                return "RGBA32UI";

            case Depth16:
                return "Depth16";
            case Depth24:
                return "Depth24";
            case Depth32F:
                return "Depth32F";

            case PixelFormat::Unknown:
                break;
        }

        return "Undefined";
    }

    struct DepthStencilState
    {
        bool      DepthTest {false};
        bool      DepthWrite {true};
        CompareOp DepthCompareOp {CompareOp::Less};

        // clang-format off
        auto operator<=> (const DepthStencilState&) const = default;
        // clang-format on
    };

    enum class BlendOp : GLenum
    {
        Add             = GL_FUNC_ADD,
        Subtract        = GL_FUNC_SUBTRACT,
        ReverseSubtract = GL_FUNC_REVERSE_SUBTRACT,
        Min             = GL_MIN,
        Max             = GL_MAX
    };
    enum class BlendFactor : GLenum
    {
        Zero                  = GL_ZERO,
        One                   = GL_ONE,
        SrcColor              = GL_SRC_COLOR,
        OneMinusSrcColor      = GL_ONE_MINUS_SRC_COLOR,
        DstColor              = GL_DST_COLOR,
        OneMinusDstColor      = GL_ONE_MINUS_DST_COLOR,
        SrcAlpha              = GL_SRC_ALPHA,
        OneMinusSrcAlpha      = GL_ONE_MINUS_SRC_ALPHA,
        DstAlpha              = GL_DST_ALPHA,
        OneMinusDstAlpha      = GL_ONE_MINUS_DST_ALPHA,
        ConstantColor         = GL_CONSTANT_COLOR,
        OneMinusConstantColor = GL_ONE_MINUS_CONSTANT_COLOR,
        ConstantAlpha         = GL_CONSTANT_ALPHA,
        OneMinusConstantAlpha = GL_ONE_MINUS_CONSTANT_ALPHA,
        SrcAlphaSaturate      = GL_SRC_ALPHA_SATURATE,
        Src1Color             = GL_SRC1_COLOR,
        OneMinusSrc1Color     = GL_ONE_MINUS_SRC1_COLOR,
        Src1Alpha             = GL_SRC1_ALPHA,
        OneMinusSrc1Alpha     = GL_ONE_MINUS_SRC1_ALPHA
    };

    // src = incoming values
    // dest = values that are already in a framebuffer
    struct BlendState
    {
        bool Enabled {false};

        BlendFactor SrcColor {BlendFactor::One};
        BlendFactor DestColor {BlendFactor::Zero};
        BlendOp     ColorOp {BlendOp::Add};

        BlendFactor SrcAlpha {BlendFactor::One};
        BlendFactor DestAlpha {BlendFactor::Zero};
        BlendOp     AlphaOp {BlendOp::Add};

        // clang-format off
        auto operator<=> (const BlendState&) const = default;
        // clang-format on
    };
    constexpr auto kMaxNumBlendStates = 4;

    enum class PolygonMode : GLenum
    {
        Point = GL_POINT,
        Line  = GL_LINE,
        Fill  = GL_FILL
    };
    enum class CullMode : GLenum
    {
        None  = GL_NONE,
        Back  = GL_BACK,
        Front = GL_FRONT
    };
    struct PolygonOffset
    {
        float Factor {0.0f}, Units {0.0f};

        // clang-format off
        auto operator<=> (const PolygonOffset&) const = default;
        // clang-format on
    };

    struct RasterizerState
    {
        PolygonMode                  PolygonMode {PolygonMode::Fill};
        CullMode                     CullMode {CullMode::Back};
        std::optional<PolygonOffset> PolygonOffset;
        bool                         DepthClampEnable {false};
        bool                         ScissorTest {false};

        // clang-format off
        auto operator<=> (const RasterizerState&) const = default;
        // clang-format on
    };

    class GraphicsPipeline
    {
    public:
        friend class RenderContext;
        GraphicsPipeline() = default;

        class Builder
        {
        public:
            Builder() = default;

            Builder& SetShaderProgram(GLuint program);
            Builder& SetVAO(GLuint vao);
            Builder& SetDepthStencil(const DepthStencilState&);
            Builder& SetRasterizerState(const RasterizerState&);
            Builder& SetBlendState(uint32_t attachment, const BlendState&);

            GraphicsPipeline Build();

        private:
            GLuint m_Program = GL_NONE;
            GLuint m_VAO     = GL_NONE;

            DepthStencilState                          m_DepthStencilState {};
            RasterizerState                            m_RasterizerState {};
            std::array<BlendState, kMaxNumBlendStates> m_BlendStates {};
        };

    private:
        Rect2D m_Viewport, m_Scissor;

        GLuint m_Program = GL_NONE;
        GLuint m_VAO     = GL_NONE;

        DepthStencilState                          m_DepthStencilState {};
        RasterizerState                            m_RasterizerState {};
        std::array<BlendState, kMaxNumBlendStates> m_BlendStates {};
    };

    GraphicsPipeline::Builder& GraphicsPipeline::Builder::SetShaderProgram(GLuint program)
    {
        m_Program = program;
        return *this;
    }

    GraphicsPipeline::Builder& GraphicsPipeline::Builder::SetVAO(GLuint vao)
    {
        m_VAO = vao;
        return *this;
    }

    GraphicsPipeline::Builder& GraphicsPipeline::Builder::SetDepthStencil(const DepthStencilState& state)
    {
        m_DepthStencilState = state;
        return *this;
    }

    GraphicsPipeline::Builder& GraphicsPipeline::Builder::SetRasterizerState(const RasterizerState& state)
    {
        m_RasterizerState = state;
        return *this;
    }

    GraphicsPipeline::Builder& GraphicsPipeline::Builder::SetBlendState(uint32_t attachment, const BlendState& state)
    {
        assert(attachment < kMaxNumBlendStates);
        m_BlendStates[attachment] = state;
        return *this;
    }

    GraphicsPipeline GraphicsPipeline::Builder::Build()
    {
        GraphicsPipeline g;

        g.m_Program           = m_Program;
        g.m_VAO               = m_VAO;
        g.m_DepthStencilState = m_DepthStencilState;
        g.m_RasterizerState   = m_RasterizerState;
        g.m_BlendStates       = m_BlendStates;

        return g;
    }

    using UniformBuffer = Buffer;
    using StorageBuffer = Buffer;

    struct ImageData
    {
        GLenum      Format {GL_NONE};
        GLenum      DataType {GL_NONE};
        const void* Pixels {nullptr};
    };

    template<typename T>
    using OptionalReference = std::optional<std::reference_wrapper<T>>;

    using ClearValue = std::variant<glm::vec4, float>;

    struct AttachmentInfo
    {
        Texture&                  Image;
        uint32_t                  MipLevel {0};
        std::optional<uint32_t>   Layer {};
        std::optional<uint32_t>   Face {};
        std::optional<ClearValue> ClearValue {};
    };
    struct RenderingInfo
    {
        Rect2D                        Area;
        std::vector<AttachmentInfo>   ColorAttachments;
        std::optional<AttachmentInfo> DepthAttachment {};
    };

    enum class PrimitiveTopology : GLenum
    {
        Undefined = GL_NONE,

        PointList = GL_POINTS,
        LineList  = GL_LINES,
        LineStrip = GL_LINE_STRIP,

        TriangleList  = GL_TRIANGLES,
        TriangleStrip = GL_TRIANGLE_STRIP,

        PatchList = GL_PATCHES
    };

    class RenderContext
    {
    public:
        RenderContext();
        ~RenderContext();

        RenderContext& SetViewport(const Rect2D& rect);
        static Rect2D  GetViewport();

        RenderContext& SetScissor(const Rect2D& rect);

        static Buffer       CreateBuffer(GLsizeiptr size, const void* data = nullptr);
        static VertexBuffer CreateVertexBuffer(GLsizei stride, int64_t capacity, const void* data = nullptr);
        static IndexBuffer  CreateIndexBuffer(IndexType, int64_t capacity, const void* data = nullptr);

        GLuint GetVertexArray(const VertexAttributes&);

        static GLuint CreateGraphicsProgram(const std::string&                vertSource,
                                            const std::string&                fragSource,
                                            const std::optional<std::string>& geomSource = std::nullopt);

        static GLuint CreateComputeProgram(const std::string& compSource);

        static Texture
        CreateTexture2D(Extent2D extent, PixelFormat, uint32_t numMipLevels = 1u, uint32_t numLayers = 0u);
        static Texture CreateTexture3D(Extent2D, uint32_t depth, PixelFormat);
        static Texture CreateCubemap(uint32_t size, PixelFormat, uint32_t numMipLevels = 1u, uint32_t numLayers = 0u);

        RenderContext& GenerateMipmaps(Texture&);

        RenderContext& SetupSampler(Texture&, const SamplerInfo&);
        static GLuint  CreateSampler(const SamplerInfo&);

        RenderContext& Clear(Texture&);
        // Upload Texture2D
        RenderContext& Upload(Texture&, GLint mipLevel, glm::uvec2 dimensions, const ImageData&);
        // Upload Cubemap face
        RenderContext& Upload(Texture&, GLint mipLevel, GLint face, glm::uvec2 dimensions, const ImageData&);
        RenderContext&
        Upload(Texture&, GLint mipLevel, const glm::uvec3& dimensions, GLint face, GLsizei layer, const ImageData&);

        RenderContext& Clear(Buffer&);
        RenderContext& Upload(Buffer&, GLintptr offset, GLsizeiptr size, const void* data);
        static void*   Map(Buffer&);
        RenderContext& Unmap(Buffer&);

        RenderContext& Destroy(Buffer&);
        RenderContext& Destroy(Texture&);
        RenderContext& Destroy(GraphicsPipeline&);

        RenderContext& Dispatch(GLuint computeProgram, const glm::uvec3& numGroups);

        GLuint         BeginRendering(const RenderingInfo& info);
        RenderContext& BeginRendering(const Rect2D&            area,
                                      std::optional<glm::vec4> clearColor   = {},
                                      std::optional<float>     clearDepth   = {},
                                      std::optional<int>       clearStencil = {});
        RenderContext& EndRendering(GLuint frameBufferID);

        RenderContext& SetUniform1f(const std::string& name, float);
        RenderContext& SetUniform1i(const std::string& name, int32_t);
        RenderContext& SetUniform1ui(const std::string& name, uint32_t);

        RenderContext& SetUniformVec3(const std::string& name, const glm::vec3&);
        RenderContext& SetUniformVec4(const std::string& name, const glm::vec4&);

        RenderContext& SetUniformMat3(const std::string& name, const glm::mat3&);
        RenderContext& SetUniformMat4(const std::string& name, const glm::mat4&);

        RenderContext& BindGraphicsPipeline(const GraphicsPipeline& gp);
        RenderContext& BindImage(GLuint unit, const Texture&, GLint mipLevel, GLenum access);
        RenderContext& BindTexture(GLuint unit, const Texture&, std::optional<GLuint> samplerId = {});
        RenderContext& BindUniformBuffer(GLuint index, const UniformBuffer&);

        RenderContext& DrawFullScreenTriangle();
        RenderContext& DrawCube();
        RenderContext& Draw(OptionalReference<const VertexBuffer> vertexBuffer,
                            OptionalReference<const IndexBuffer>  indexBuffer,
                            uint32_t                              numIndices,
                            uint32_t                              numVertices,
                            uint32_t                              numInstances = 1);

        struct ResourceDeleter
        {
            void operator()(auto* ptr)
            {
                Context.Destroy(*ptr);
                delete ptr;
            }

            RenderContext& Context;
        };

    private:
        static GLuint CreateVertexArray(const VertexAttributes&);

        static Texture CreateImmutableTexture(Extent2D,
                                              uint32_t depth,
                                              PixelFormat,
                                              uint32_t numFaces,
                                              uint32_t numMipLevels,
                                              uint32_t numLayers);

        static void CreateFaceView(Texture& cubeMap, GLuint mipLevel, GLuint layer, GLuint face);
        static void AttachTexture(GLuint framebuffer, GLenum attachment, const AttachmentInfo&);

        static GLuint CreateShaderProgram(std::initializer_list<GLuint> shaders);
        static GLuint CreateShaderObject(GLenum type, const std::string& shaderSource);

        void SetShaderProgram(GLuint);
        void SetVertexArray(GLuint);
        void SetVertexBuffer(const VertexBuffer&) const;
        void SetIndexBuffer(const IndexBuffer&) const;

        void SetDepthTest(bool enabled, CompareOp);
        void SetDepthWrite(bool enabled);

        void SetPolygonMode(PolygonMode);
        void SetPolygonOffset(std::optional<PolygonOffset>);
        void SetCullMode(CullMode);
        void SetDepthClamp(bool enabled);
        void SetScissorTest(bool enabled);

        void SetBlendState(GLuint index, const BlendState&);

    private:
        bool             m_RenderingStarted = false;
        GraphicsPipeline m_CurrentPipeline;

        GLuint                                  m_DummyVAO {GL_NONE};
        std::unordered_map<std::size_t, GLuint> m_VertexArrays;
    };

    // @return {data type, number of components, normalize}
    std::tuple<GLenum, GLint, GLboolean> statAttribute(VertexAttribute::Type type)
    {
        switch (type)
        {
            using enum VertexAttribute::Type;
            case Float:
                return {GL_FLOAT, 1, GL_FALSE};
            case Float2:
                return {GL_FLOAT, 2, GL_FALSE};
            case Float3:
                return {GL_FLOAT, 3, GL_FALSE};
            case Float4:
                return {GL_FLOAT, 4, GL_FALSE};

            case Int:
                return {GL_INT, 1, GL_FALSE};
            case Int4:
                return {GL_INT, 4, GL_FALSE};

            case UByte4_Norm:
                return {GL_UNSIGNED_BYTE, 4, GL_TRUE};
        }
        return {GL_INVALID_INDEX, 0, GL_FALSE};
    }

    GLenum selectTextureMinFilter(TexelFilter minFilter, MipmapMode mipmapMode)
    {
        GLenum result {GL_NONE};
        switch (minFilter)
        {
            case TexelFilter::Nearest:
                switch (mipmapMode)
                {
                    case MipmapMode::None:
                        result = GL_NEAREST;
                        break;
                    case MipmapMode::Nearest:
                        result = GL_NEAREST_MIPMAP_NEAREST;
                        break;
                    case MipmapMode::Linear:
                        result = GL_NEAREST_MIPMAP_LINEAR;
                        break;
                }
                break;

            case TexelFilter::Linear:
                switch (mipmapMode)
                {
                    case MipmapMode::None:
                        result = GL_LINEAR;
                        break;
                    case MipmapMode::Nearest:
                        result = GL_LINEAR_MIPMAP_NEAREST;
                        break;
                    case MipmapMode::Linear:
                        result = GL_LINEAR_MIPMAP_LINEAR;
                        break;
                }
                break;
        }
        assert(result != GL_NONE);
        return result;
    }
    GLenum getIndexDataType(GLsizei stride)
    {
        switch (stride)
        {
            case sizeof(GLubyte):
                return GL_UNSIGNED_BYTE;
            case sizeof(GLushort):
                return GL_UNSIGNED_SHORT;
            case sizeof(GLuint):
                return GL_UNSIGNED_INT;
        }
        assert(false);
        return GL_NONE;
    }

    GLenum getPolygonOffsetCap(PolygonMode polygonMode)
    {
        switch (polygonMode)
        {
            case PolygonMode::Fill:
                return GL_POLYGON_OFFSET_FILL;
            case PolygonMode::Line:
                return GL_POLYGON_OFFSET_LINE;
            case PolygonMode::Point:
                return GL_POLYGON_OFFSET_POINT;
        }
        assert(false);
        return GL_NONE;
    }

    RenderContext::RenderContext() { glCreateVertexArrays(1, &m_DummyVAO); }

    RenderContext::~RenderContext()
    {
        glDeleteVertexArrays(1, &m_DummyVAO);
        for (auto [_, vao] : m_VertexArrays)
            glDeleteVertexArrays(1, &vao);

        m_CurrentPipeline = {};
    }

    RenderContext& RenderContext::SetViewport(const Rect2D& rect)
    {
        auto& current = m_CurrentPipeline.m_Viewport;
        if (rect != current)
        {
            glViewport(rect.Offset.X, rect.Offset.Y, rect.Extent.Width, rect.Extent.Height);
            current = rect;
        }
        return *this;
    }

    Rect2D RenderContext::GetViewport()
    {
        GLint viewport[4];
        glGetIntegerv(GL_VIEWPORT, viewport);

        return {{viewport[0], viewport[1]}, {static_cast<uint32_t>(viewport[2]), static_cast<uint32_t>(viewport[3])}};
    }

    RenderContext& RenderContext::SetScissor(const Rect2D& rect)
    {
        auto& current = m_CurrentPipeline.m_Scissor;
        if (rect != current)
        {
            glScissor(rect.Offset.X, rect.Offset.Y, rect.Extent.Width, rect.Extent.Height);
            current = rect;
        }
        return *this;
    }

    Buffer RenderContext::CreateBuffer(GLsizeiptr size, const void* data)
    {
        GLuint buffer;
        glCreateBuffers(1, &buffer);
        glNamedBufferStorage(buffer, size, data, GL_DYNAMIC_STORAGE_BIT);

        return {buffer, size};
    }

    VertexBuffer RenderContext::CreateVertexBuffer(GLsizei stride, int64_t capacity, const void* data)
    {
        return VertexBuffer {CreateBuffer(stride * capacity, data), stride};
    }

    IndexBuffer RenderContext::CreateIndexBuffer(IndexType indexType, int64_t capacity, const void* data)
    {
        const auto stride = static_cast<GLsizei>(indexType);
        return IndexBuffer {CreateBuffer(stride * capacity, data), indexType};
    }

    GLuint RenderContext::GetVertexArray(const VertexAttributes& attributes)
    {
        assert(!attributes.empty());

        std::size_t hash {0};
        for (const auto& [location, attribute] : attributes)
            utils::hashCombine(hash, location, attribute);

        auto it = m_VertexArrays.find(hash);
        if (it == m_VertexArrays.cend())
        {
            it = m_VertexArrays.emplace(hash, CreateVertexArray(attributes)).first;
            std::cout << "[RenderContext] Created VAO: " << hash << std::endl;
        }

        return it->second;
    }

    GLuint RenderContext::CreateGraphicsProgram(const std::string&                vertSource,
                                                const std::string&                fragSource,
                                                const std::optional<std::string>& geomSource)
    {
        return CreateShaderProgram({
            CreateShaderObject(GL_VERTEX_SHADER, vertSource),
            geomSource ? CreateShaderObject(GL_GEOMETRY_SHADER, *geomSource) : GL_NONE,
            CreateShaderObject(GL_FRAGMENT_SHADER, fragSource),
        });
    }

    GLuint RenderContext::CreateComputeProgram(const std::string& compSource)
    {
        return CreateShaderProgram({
            CreateShaderObject(GL_COMPUTE_SHADER, compSource),
        });
    }

    Texture
    RenderContext::CreateTexture2D(Extent2D extent, PixelFormat pixelFormat, uint32_t numMipLevels, uint32_t numLayers)
    {
        assert(extent.Width > 0 && extent.Height > 0 && pixelFormat != PixelFormat::Unknown);

        if (numMipLevels <= 0)
            numMipLevels = calcMipLevels(glm::max(extent.Width, extent.Height));

        return CreateImmutableTexture(extent, 0, pixelFormat, 1, numMipLevels, numLayers);
    }

    Texture RenderContext::CreateTexture3D(Extent2D extent, uint32_t depth, PixelFormat pixelFormat)
    {
        return CreateImmutableTexture(extent, depth, pixelFormat, 1, 1, 0);
    }

    Texture
    RenderContext::CreateCubemap(uint32_t size, PixelFormat pixelFormat, uint32_t numMipLevels, uint32_t numLayers)
    {
        assert(size > 0 && pixelFormat != PixelFormat::Unknown);

        if (numMipLevels <= 0)
            numMipLevels = calcMipLevels(size);

        return CreateImmutableTexture({size, size}, 0, pixelFormat, 6, numMipLevels, numLayers);
    }

    RenderContext& RenderContext::GenerateMipmaps(Texture& texture)
    {
        assert(texture);
        glGenerateTextureMipmap(texture.m_Id);

        return *this;
    }

    RenderContext& RenderContext::SetupSampler(Texture& texture, const SamplerInfo& samplerInfo)
    {
        assert(texture);

        glTextureParameteri(
            texture.m_Id, GL_TEXTURE_MIN_FILTER, selectTextureMinFilter(samplerInfo.MinFilter, samplerInfo.MipmapMode));
        glTextureParameteri(texture.m_Id, GL_TEXTURE_MAG_FILTER, static_cast<GLenum>(samplerInfo.MagFilter));
        glTextureParameteri(texture.m_Id, GL_TEXTURE_WRAP_S, static_cast<GLenum>(samplerInfo.AddressModeS));
        glTextureParameteri(texture.m_Id, GL_TEXTURE_WRAP_T, static_cast<GLenum>(samplerInfo.AddressModeT));
        glTextureParameteri(texture.m_Id, GL_TEXTURE_WRAP_R, static_cast<GLenum>(samplerInfo.AddressModeR));

        glTextureParameterf(texture.m_Id, GL_TEXTURE_MAX_ANISOTROPY, samplerInfo.MaxAnisotropy);

        if (samplerInfo.CompareOperator.has_value())
        {
            glTextureParameteri(texture.m_Id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glTextureParameteri(
                texture.m_Id, GL_TEXTURE_COMPARE_FUNC, static_cast<GLenum>(*samplerInfo.CompareOperator));
        }
        glTextureParameterfv(texture.m_Id, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(samplerInfo.BorderColor));

        return *this;
    }

    GLuint RenderContext::CreateSampler(const SamplerInfo& samplerInfo)
    {
        GLuint sampler {GL_NONE};
        glCreateSamplers(1, &sampler);

        glSamplerParameteri(
            sampler, GL_TEXTURE_MIN_FILTER, selectTextureMinFilter(samplerInfo.MinFilter, samplerInfo.MipmapMode));
        glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, static_cast<GLenum>(samplerInfo.MagFilter));
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, static_cast<GLenum>(samplerInfo.AddressModeS));
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, static_cast<GLenum>(samplerInfo.AddressModeT));
        glSamplerParameteri(sampler, GL_TEXTURE_WRAP_R, static_cast<GLenum>(samplerInfo.AddressModeR));

        glSamplerParameterf(sampler, GL_TEXTURE_MAX_ANISOTROPY, samplerInfo.MaxAnisotropy);

        if (samplerInfo.CompareOperator.has_value())
        {
            glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
            glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_FUNC, static_cast<GLenum>(*samplerInfo.CompareOperator));
        }
        glSamplerParameterfv(sampler, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(samplerInfo.BorderColor));

        return sampler;
    }

    RenderContext& RenderContext::Clear(Texture& texture)
    {
        assert(texture);
        uint8_t v {0};
        glClearTexImage(texture.m_Id, 0, GL_RED, GL_UNSIGNED_BYTE, &v);

        return *this;
    }

    RenderContext&
    RenderContext::Upload(Texture& texture, GLint mipLevel, glm::uvec2 dimensions, const ImageData& image)
    {
        return Upload(texture, mipLevel, {dimensions, 0}, 0, 0, image);
    }

    RenderContext&
    RenderContext::Upload(Texture& texture, GLint mipLevel, GLint face, glm::uvec2 dimensions, const ImageData& image)
    {
        return Upload(texture, mipLevel, {dimensions, 0}, face, 0, image);
    }

    RenderContext& RenderContext::Upload(Texture&          texture,
                                         GLint             mipLevel,
                                         const glm::uvec3& dimensions,
                                         GLint             face,
                                         GLsizei           layer,
                                         const ImageData&  image)
    {
        assert(texture && image.Pixels != nullptr);

        switch (texture.m_Type)
        {
            case GL_TEXTURE_1D:
                glTextureSubImage1D(
                    texture.m_Id, mipLevel, 0, dimensions.x, image.Format, image.DataType, image.Pixels);
                break;
            case GL_TEXTURE_1D_ARRAY:
                glTextureSubImage2D(
                    texture.m_Id, mipLevel, 0, 0, dimensions.x, layer, image.Format, image.DataType, image.Pixels);
                break;
            case GL_TEXTURE_2D:
                glTextureSubImage2D(texture.m_Id,
                                    mipLevel,
                                    0,
                                    0,
                                    dimensions.x,
                                    dimensions.y,
                                    image.Format,
                                    image.DataType,
                                    image.Pixels);
                break;
            case GL_TEXTURE_2D_ARRAY:
                glTextureSubImage3D(texture.m_Id,
                                    mipLevel,
                                    0,
                                    0,
                                    layer,
                                    dimensions.x,
                                    dimensions.y,
                                    1,
                                    image.Format,
                                    image.DataType,
                                    image.Pixels);
                break;
            case GL_TEXTURE_3D:
                glTextureSubImage3D(texture.m_Id,
                                    mipLevel,
                                    0,
                                    0,
                                    0,
                                    dimensions.x,
                                    dimensions.y,
                                    dimensions.z,
                                    image.Format,
                                    image.DataType,
                                    image.Pixels);
                break;
            case GL_TEXTURE_CUBE_MAP:
                glTextureSubImage3D(texture.m_Id,
                                    mipLevel,
                                    0,
                                    0,
                                    face,
                                    dimensions.x,
                                    dimensions.y,
                                    1,
                                    image.Format,
                                    image.DataType,
                                    image.Pixels);
                break;
            case GL_TEXTURE_CUBE_MAP_ARRAY: {
                const auto zoffset = (layer * 6) + face; // desired layer-face
                // depth = how many layer-faces
                glTextureSubImage3D(texture.m_Id,
                                    mipLevel,
                                    0,
                                    0,
                                    zoffset,
                                    dimensions.x,
                                    dimensions.y,
                                    6 * texture.m_NumLayers,
                                    image.Format,
                                    image.DataType,
                                    image.Pixels);
            }
            break;

            default:
                assert(false);
        }
        return *this;
    }

    RenderContext& RenderContext::Clear(Buffer& buffer)
    {
        assert(buffer);

        uint8_t v {0};
        glClearNamedBufferData(buffer.m_Id, GL_R8, GL_RED, GL_UNSIGNED_BYTE, &v);

        return *this;
    }

    RenderContext& RenderContext::Upload(Buffer& buffer, GLintptr offset, GLsizeiptr size, const void* data)
    {
        assert(buffer);

        if (size > 0 && data != nullptr)
            glNamedBufferSubData(buffer.m_Id, offset, size, data);

        return *this;
    }

    void* RenderContext::Map(Buffer& buffer)
    {
        assert(buffer);

        if (!buffer.IsMapped())
            buffer.m_MappedMemory = glMapNamedBuffer(buffer.m_Id, GL_WRITE_ONLY);

        return buffer.m_MappedMemory;
    }

    RenderContext& RenderContext::Unmap(Buffer& buffer)
    {
        assert(buffer);

        if (buffer.IsMapped())
        {
            glUnmapNamedBuffer(buffer.m_Id);
            buffer.m_MappedMemory = nullptr;
        }

        return *this;
    }

    RenderContext& RenderContext::Destroy(Buffer& buffer)
    {
        if (buffer)
        {
            glDeleteBuffers(1, &buffer.m_Id);
            buffer = {};
        }

        return *this;
    }

    RenderContext& RenderContext::Destroy(Texture& texture)
    {
        if (texture)
        {
            glDeleteTextures(1, &texture.m_Id);
            if (texture.m_View != GL_NONE)
                glDeleteTextures(1, &texture.m_View);
            texture = {};
        }
        return *this;
    }

    RenderContext& RenderContext::Destroy(GraphicsPipeline& gp)
    {
        if (gp.m_Program != GL_NONE)
        {
            glDeleteProgram(gp.m_Program);
            gp.m_Program = GL_NONE;
        }
        gp.m_VAO = GL_NONE;

        return *this;
    }

    RenderContext& RenderContext::Dispatch(GLuint computeProgram, const glm::uvec3& numGroups)
    {
        SetShaderProgram(computeProgram);
        glDispatchCompute(numGroups.x, numGroups.y, numGroups.z);

        return *this;
    }

    GLuint RenderContext::BeginRendering(const RenderingInfo& renderingInfo)
    {
        assert(!m_RenderingStarted);

        GLuint framebuffer;
        glCreateFramebuffers(1, &framebuffer);
        if (renderingInfo.DepthAttachment.has_value())
        {
            AttachTexture(framebuffer, GL_DEPTH_ATTACHMENT, *renderingInfo.DepthAttachment);
        }
        for (size_t i {0}; i < renderingInfo.ColorAttachments.size(); ++i)
        {
            AttachTexture(framebuffer, GL_COLOR_ATTACHMENT0 + i, renderingInfo.ColorAttachments[i]);
        }
        if (const auto n = renderingInfo.ColorAttachments.size(); n > 0)
        {
            std::vector<GLenum> colorBuffers(n);
            std::iota(colorBuffers.begin(), colorBuffers.end(), GL_COLOR_ATTACHMENT0);
            glNamedFramebufferDrawBuffers(framebuffer, colorBuffers.size(), colorBuffers.data());
        }
#ifdef _DEBUG
        const auto status = glCheckNamedFramebufferStatus(framebuffer, GL_DRAW_FRAMEBUFFER);
        assert(GL_FRAMEBUFFER_COMPLETE == status);
#endif

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);
        SetViewport(renderingInfo.Area);
        SetScissorTest(false);

        if (renderingInfo.DepthAttachment.has_value())
            if (renderingInfo.DepthAttachment->ClearValue.has_value())
            {
                SetDepthWrite(true);

                const auto clearValue = std::get<float>(*renderingInfo.DepthAttachment->ClearValue);
                glClearNamedFramebufferfv(framebuffer, GL_DEPTH, 0, &clearValue);
            }
        for (int32_t i {0}; const auto& attachment : renderingInfo.ColorAttachments)
        {
            if (attachment.ClearValue.has_value())
            {
                const auto& clearValue = std::get<glm::vec4>(*attachment.ClearValue);
                glClearNamedFramebufferfv(framebuffer, GL_COLOR, i, glm::value_ptr(clearValue));
            }
            ++i;
        }

        m_RenderingStarted = true;

        return framebuffer;
    }

    RenderContext& RenderContext::BeginRendering(const Rect2D&            area,
                                                 std::optional<glm::vec4> clearColor,
                                                 std::optional<float>     clearDepth,
                                                 std::optional<int>       clearStencil)
    {
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GL_NONE);
        SetViewport(area);
        SetScissorTest(false);

        if (clearDepth.has_value())
        {
            SetDepthWrite(true);
            glClearNamedFramebufferfv(GL_NONE, GL_DEPTH, 0, &clearDepth.value());
        }
        if (clearStencil.has_value())
            glClearNamedFramebufferiv(GL_NONE, GL_STENCIL, 0, &clearStencil.value());
        if (clearColor.has_value())
        {
            glClearNamedFramebufferfv(GL_NONE, GL_COLOR, 0, glm::value_ptr(clearColor.value()));
        }

        return *this;
    }

    RenderContext& RenderContext::EndRendering(GLuint frameBufferID)
    {
        assert(m_RenderingStarted && frameBufferID != GL_NONE);

        glDeleteFramebuffers(1, &frameBufferID);
        m_RenderingStarted = false;

        return *this;
    }

    RenderContext& RenderContext::BindGraphicsPipeline(const GraphicsPipeline& gp)
    {
        {
            const auto& state = gp.m_DepthStencilState;
            SetDepthTest(state.DepthTest, state.DepthCompareOp);
            SetDepthWrite(state.DepthWrite);
        }

        {
            const auto& state = gp.m_RasterizerState;
            SetPolygonMode(state.PolygonMode);
            SetCullMode(state.CullMode);
            SetPolygonOffset(state.PolygonOffset);
            SetDepthClamp(state.DepthClampEnable);
            SetScissorTest(state.ScissorTest);
        }

        for (int32_t i {0}; i < gp.m_BlendStates.size(); ++i)
            SetBlendState(i, gp.m_BlendStates[i]);

        SetVertexArray(gp.m_VAO);
        SetShaderProgram(gp.m_Program);

        return *this;
    }

    RenderContext& RenderContext::SetUniform1f(const std::string& name, float f)
    {
        const auto location = glGetUniformLocation(m_CurrentPipeline.m_Program, name.data());
        if (location != GL_INVALID_INDEX)
            glProgramUniform1f(m_CurrentPipeline.m_Program, location, f);
        return *this;
    }

    RenderContext& RenderContext::SetUniform1i(const std::string& name, int32_t i)
    {
        const auto location = glGetUniformLocation(m_CurrentPipeline.m_Program, name.data());
        if (location != GL_INVALID_INDEX)
            glProgramUniform1i(m_CurrentPipeline.m_Program, location, i);
        return *this;
    }

    RenderContext& RenderContext::SetUniform1ui(const std::string& name, uint32_t i)
    {
        const auto location = glGetUniformLocation(m_CurrentPipeline.m_Program, name.data());
        if (location != GL_INVALID_INDEX)
            glProgramUniform1ui(m_CurrentPipeline.m_Program, location, i);
        return *this;
    }

    RenderContext& RenderContext::SetUniformVec3(const std::string& name, const glm::vec3& v)
    {
        const auto location = glGetUniformLocation(m_CurrentPipeline.m_Program, name.data());
        if (location != GL_INVALID_INDEX)
        {
            glProgramUniform3fv(m_CurrentPipeline.m_Program, location, 1, glm::value_ptr(v));
        }
        return *this;
    }

    RenderContext& RenderContext::SetUniformVec4(const std::string& name, const glm::vec4& v)
    {
        const auto location = glGetUniformLocation(m_CurrentPipeline.m_Program, name.data());
        if (location != GL_INVALID_INDEX)
        {
            glProgramUniform4fv(m_CurrentPipeline.m_Program, location, 1, glm::value_ptr(v));
        }
        return *this;
    }

    RenderContext& RenderContext::SetUniformMat3(const std::string& name, const glm::mat3& m)
    {
        const auto location = glGetUniformLocation(m_CurrentPipeline.m_Program, name.data());
        if (location != GL_INVALID_INDEX)
        {
            glProgramUniformMatrix3fv(m_CurrentPipeline.m_Program, location, 1, GL_FALSE, glm::value_ptr(m));
        }
        return *this;
    }

    RenderContext& RenderContext::SetUniformMat4(const std::string& name, const glm::mat4& m)
    {
        const auto location = glGetUniformLocation(m_CurrentPipeline.m_Program, name.data());
        if (location != GL_INVALID_INDEX)
        {
            glProgramUniformMatrix4fv(m_CurrentPipeline.m_Program, location, 1, GL_FALSE, glm::value_ptr(m));
        }
        return *this;
    }

    RenderContext& RenderContext::BindImage(GLuint unit, const Texture& texture, GLint mipLevel, GLenum access)
    {
        assert(texture && mipLevel < texture.m_NumMipLevels);
        glBindImageTexture(
            unit, texture.m_Id, mipLevel, GL_FALSE, 0, access, static_cast<GLenum>(texture.m_PixelFormat));
        return *this;
    }

    RenderContext& RenderContext::BindTexture(GLuint unit, const Texture& texture, std::optional<GLuint> samplerId)
    {
        assert(texture);
        glBindTextureUnit(unit, texture.m_Id);
        if (samplerId.has_value())
            glBindSampler(unit, *samplerId);
        return *this;
    }

    RenderContext& RenderContext::BindUniformBuffer(GLuint index, const UniformBuffer& buffer)
    {
        assert(buffer);
        glBindBufferBase(GL_UNIFORM_BUFFER, index, buffer.m_Id);
        return *this;
    }

    RenderContext& RenderContext::DrawFullScreenTriangle() { return Draw({}, {}, 0, 3); }

    RenderContext& RenderContext::DrawCube() { return Draw({}, {}, 0, 36); }

    RenderContext& RenderContext::Draw(OptionalReference<const VertexBuffer> vertexBuffer,
                                       OptionalReference<const IndexBuffer>  indexBuffer,
                                       uint32_t                              numIndices,
                                       uint32_t                              numVertices,
                                       uint32_t                              numInstances)
    {
        if (vertexBuffer.has_value())
            SetVertexBuffer(*vertexBuffer);

        if (numIndices > 0)
        {
            assert(indexBuffer.has_value());
            SetIndexBuffer(*indexBuffer);
            glDrawElementsInstanced(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, nullptr, numInstances);
        }
        else
        {
            glDrawArraysInstanced(GL_TRIANGLES, 0, numVertices, numInstances);
        }
        return *this;
    }

    GLuint RenderContext::CreateVertexArray(const VertexAttributes& attributes)
    {
        GLuint vao;
        glCreateVertexArrays(1, &vao);

        for (const auto& [location, attribute] : attributes)
        {
            const auto [type, size, normalized] = statAttribute(attribute.VertType);
            assert(type != GL_INVALID_INDEX);

            glEnableVertexArrayAttrib(vao, location);
            if (attribute.VertType == VertexAttribute::Type::Int4)
            {
                glVertexArrayAttribIFormat(vao, location, size, type, attribute.Offset);
            }
            else
            {
                glVertexArrayAttribFormat(vao, location, size, type, normalized, attribute.Offset);
            }
            glVertexArrayAttribBinding(vao, location, 0);
        }
        return vao;
    }

    Texture RenderContext::CreateImmutableTexture(Extent2D    extent,
                                                  uint32_t    depth,
                                                  PixelFormat pixelFormat,
                                                  uint32_t    numFaces,
                                                  uint32_t    numMipLevels,
                                                  uint32_t    numLayers)
    {
        assert(numMipLevels > 0);

        // http://github.khronos.org/KTX-Specification/#_texture_type

        GLenum target = numFaces == 6     ? GL_TEXTURE_CUBE_MAP :
                        depth > 0         ? GL_TEXTURE_3D :
                        extent.Height > 0 ? GL_TEXTURE_2D :
                                            GL_TEXTURE_1D;
        assert(target == GL_TEXTURE_CUBE_MAP ? extent.Width == extent.Height : true);

        if (numLayers > 0)
        {
            switch (target)
            {
                case GL_TEXTURE_1D:
                    target = GL_TEXTURE_1D_ARRAY;
                    break;
                case GL_TEXTURE_2D:
                    target = GL_TEXTURE_2D_ARRAY;
                    break;
                case GL_TEXTURE_CUBE_MAP:
                    target = GL_TEXTURE_CUBE_MAP_ARRAY;
                    break;

                default:
                    assert(false);
            }
        }

        GLuint id {GL_NONE};
        glCreateTextures(target, 1, &id);

        const auto internalFormat = static_cast<GLenum>(pixelFormat);
        switch (target)
        {
            case GL_TEXTURE_1D:
                glTextureStorage1D(id, numMipLevels, internalFormat, extent.Width);
                break;
            case GL_TEXTURE_1D_ARRAY:
                glTextureStorage2D(id, numMipLevels, internalFormat, extent.Width, numLayers);
                break;

            case GL_TEXTURE_2D:
                glTextureStorage2D(id, numMipLevels, internalFormat, extent.Width, extent.Height);
                break;
            case GL_TEXTURE_2D_ARRAY:
                glTextureStorage3D(id, numMipLevels, internalFormat, extent.Width, extent.Height, numLayers);
                break;

            case GL_TEXTURE_3D:
                glTextureStorage3D(id, numMipLevels, internalFormat, extent.Width, extent.Height, depth);
                break;

            case GL_TEXTURE_CUBE_MAP:
                glTextureStorage2D(id, numMipLevels, internalFormat, extent.Width, extent.Height);
                break;
            case GL_TEXTURE_CUBE_MAP_ARRAY:
                glTextureStorage3D(id, numMipLevels, internalFormat, extent.Width, extent.Height, numLayers * 6);
                break;
        }

        return Texture {
            id,
            target,
            pixelFormat,
            extent,
            depth,
            numMipLevels,
            numLayers,
        };
    }

    void RenderContext::CreateFaceView(Texture& cubeMap, GLuint mipLevel, GLuint layer, GLuint face)
    {
        assert(cubeMap.m_Type == GL_TEXTURE_CUBE_MAP || cubeMap.m_Type == GL_TEXTURE_CUBE_MAP_ARRAY);

        if (cubeMap.m_View != GL_NONE)
            glDeleteTextures(1, &cubeMap.m_View);
        glGenTextures(1, &cubeMap.m_View);

        glTextureView(cubeMap.m_View,
                      GL_TEXTURE_2D,
                      cubeMap.m_Id,
                      static_cast<GLenum>(cubeMap.m_PixelFormat),
                      mipLevel,
                      1,
                      (layer * 6) + face,
                      1);
    }

    void RenderContext::AttachTexture(GLuint framebuffer, GLenum attachment, const AttachmentInfo& info)
    {
        const auto& [image, mipLevel, maybeLayer, maybeFace, _] = info;

        switch (image.m_Type)
        {
            case GL_TEXTURE_CUBE_MAP:
            case GL_TEXTURE_CUBE_MAP_ARRAY:
                CreateFaceView(image, mipLevel, maybeLayer.value_or(0), maybeFace.value_or(0));
                glNamedFramebufferTexture(framebuffer, attachment, image.m_View, 0);
                break;

            case GL_TEXTURE_2D:
                glNamedFramebufferTexture(framebuffer, attachment, image.m_Id, mipLevel);
                break;

            case GL_TEXTURE_2D_ARRAY:
                assert(maybeLayer.has_value());
                glNamedFramebufferTextureLayer(framebuffer, attachment, image.m_Id, mipLevel, maybeLayer.value_or(0));
                break;

            case GL_TEXTURE_3D:
                glNamedFramebufferTexture(framebuffer, attachment, image.m_Id, 0);
                break;

            default:
                assert(false);
        }
    }

    GLuint RenderContext::CreateShaderProgram(std::initializer_list<GLuint> shaders)
    {
        const auto program = glCreateProgram();

        for (auto shader : shaders)
            if (shader != GL_NONE)
                glAttachShader(program, shader);

        glLinkProgram(program);

        GLint status;
        glGetProgramiv(program, GL_LINK_STATUS, &status);
        if (GL_FALSE == status)
        {
            GLint infoLogLength;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);
            assert(infoLogLength > 0);
            std::string infoLog("", infoLogLength);
            glGetProgramInfoLog(program, infoLogLength, nullptr, infoLog.data());
            throw std::runtime_error {infoLog};
        }
        for (auto shader : shaders)
        {
            if (shader != GL_NONE)
            {
                glDetachShader(program, shader);
                glDeleteShader(shader);
            }
        }

        return program;
    }

    GLuint RenderContext::CreateShaderObject(GLenum type, const std::string& shaderSource)
    {
        auto          id = glCreateShader(type);
        const GLchar* strings {shaderSource.data()};
        glShaderSource(id, 1, &strings, nullptr);
        glCompileShader(id);

        GLint status;
        glGetShaderiv(id, GL_COMPILE_STATUS, &status);
        if (GL_FALSE == status)
        {
            GLint infoLogLength;
            glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLogLength);
            assert(infoLogLength > 0);
            std::string infoLog("", infoLogLength);
            glGetShaderInfoLog(id, infoLogLength, nullptr, infoLog.data());
            throw std::runtime_error {infoLog};
        };

        return id;
    }

    void RenderContext::SetShaderProgram(GLuint program)
    {
        assert(program != GL_NONE);
        if (auto& current = m_CurrentPipeline.m_Program; current != program)
        {
            glUseProgram(program);
            current = program;
        }
    }

    void RenderContext::SetVertexArray(GLuint vao)
    {
        if (GL_NONE == vao)
            vao = m_DummyVAO;

        if (auto& current = m_CurrentPipeline.m_VAO; vao != current)
        {
            glBindVertexArray(vao);
            current = vao;
        }
    }

    void RenderContext::SetVertexBuffer(const VertexBuffer& vertexBuffer) const
    {
        const auto vao = m_CurrentPipeline.m_VAO;
        assert(vertexBuffer && vao != GL_NONE);
        glVertexArrayVertexBuffer(vao, 0, vertexBuffer.m_Id, 0, vertexBuffer.GetStride());
    }

    void RenderContext::SetIndexBuffer(const IndexBuffer& indexBuffer) const
    {
        const auto vao = m_CurrentPipeline.m_VAO;
        assert(indexBuffer && vao != GL_NONE);
        glVertexArrayElementBuffer(vao, indexBuffer.m_Id);
    }

    void RenderContext::SetDepthTest(bool enabled, CompareOp depthFunc)
    {
        auto& current = m_CurrentPipeline.m_DepthStencilState;
        if (enabled != current.DepthTest)
        {
            enabled ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
            current.DepthTest = enabled;
        }
        if (enabled && depthFunc != current.DepthCompareOp)
        {
            glDepthFunc(static_cast<GLenum>(depthFunc));
            current.DepthCompareOp = depthFunc;
        }
    }

    void RenderContext::SetDepthWrite(bool enabled)
    {
        auto& current = m_CurrentPipeline.m_DepthStencilState;
        if (enabled != current.DepthWrite)
        {
            glDepthMask(enabled);
            current.DepthWrite = enabled;
        }
    }

    void RenderContext::SetPolygonMode(PolygonMode polygonMode)
    {
        auto& current = m_CurrentPipeline.m_RasterizerState.PolygonMode;
        if (polygonMode != current)
        {
            glPolygonMode(GL_FRONT_AND_BACK, static_cast<GLenum>(polygonMode));
            current = polygonMode;
        }
    }

    void RenderContext::SetPolygonOffset(std::optional<PolygonOffset> polygonOffset)
    {
        auto& current = m_CurrentPipeline.m_RasterizerState;
        if (polygonOffset != current.PolygonOffset)
        {
            const auto offsetCap = getPolygonOffsetCap(current.PolygonMode);
            if (polygonOffset.has_value())
            {
                glEnable(offsetCap);
                glPolygonOffset(polygonOffset->Factor, polygonOffset->Units);
            }
            else
            {
                glDisable(offsetCap);
            }
            current.PolygonOffset = polygonOffset;
        }
    }

    void RenderContext::SetCullMode(CullMode cullMode)
    {
        auto& current = m_CurrentPipeline.m_RasterizerState.CullMode;
        if (cullMode != current)
        {
            if (cullMode != CullMode::None)
            {
                if (current == CullMode::None)
                    glEnable(GL_CULL_FACE);
                glCullFace(static_cast<GLenum>(cullMode));
            }
            else
            {
                glDisable(GL_CULL_FACE);
            }
            current = cullMode;
        }
    }

    void RenderContext::SetDepthClamp(bool enabled)
    {
        auto& current = m_CurrentPipeline.m_RasterizerState.DepthClampEnable;
        if (enabled != current)
        {
            enabled ? glEnable(GL_DEPTH_CLAMP) : glDisable(GL_DEPTH_CLAMP);
            current = enabled;
        }
    }

    void RenderContext::SetScissorTest(bool enabled)
    {
        auto& current = m_CurrentPipeline.m_RasterizerState.ScissorTest;
        if (enabled != current)
        {
            enabled ? glEnable(GL_SCISSOR_TEST) : glDisable(GL_SCISSOR_TEST);
            current = enabled;
        }
    }

    void RenderContext::SetBlendState(GLuint index, const BlendState& state)
    {
        auto& current = m_CurrentPipeline.m_BlendStates[index];
        if (state != current)
        {
            if (state.Enabled != current.Enabled)
            {
                state.Enabled ? glEnablei(GL_BLEND, index) : glDisablei(GL_BLEND, index);
                current.Enabled = state.Enabled;
            }
            if (state.Enabled)
            {
                if (state.ColorOp != current.ColorOp || state.AlphaOp != current.AlphaOp)
                {
                    glBlendEquationSeparatei(
                        index, static_cast<GLenum>(state.ColorOp), static_cast<GLenum>(state.AlphaOp));

                    current.ColorOp = state.ColorOp;
                    current.AlphaOp = state.AlphaOp;
                }
                if (state.SrcColor != current.SrcColor || state.DestColor != current.DestColor ||
                    state.SrcAlpha != current.SrcAlpha || state.DestAlpha != current.DestAlpha)
                {
                    glBlendFuncSeparatei(index,
                                         static_cast<GLenum>(state.SrcColor),
                                         static_cast<GLenum>(state.DestColor),
                                         static_cast<GLenum>(state.SrcAlpha),
                                         static_cast<GLenum>(state.DestAlpha));

                    current.SrcColor  = state.SrcColor;
                    current.DestColor = state.DestColor;
                    current.SrcAlpha  = state.SrcAlpha;
                    current.DestAlpha = state.DestAlpha;
                }
            }
        }
    }
} // namespace vgfw

namespace std
{
    template<>
    struct hash<vgfw::VertexAttribute>
    {
        std::size_t operator()(const vgfw::VertexAttribute& attribute) const noexcept
        {
            std::size_t h {0};
            vgfw::utils::hashCombine(h, attribute.VertType, attribute.Offset);
            return h;
        }
    };
} // namespace std