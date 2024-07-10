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

#define VGFW_RENDER_API_OPENGL_MIN_MAJOR 3
#define VGFW_RENDER_API_OPENGL_MIN_MINOR 3

// force OpenGL 4.6 on Windows
#if VGFW_PLATFORM_WINDOWS
#undef VGFW_RENDER_API_OPENGL_MIN_MAJOR
#undef VGFW_RENDER_API_OPENGL_MIN_MINOR
#define VGFW_RENDER_API_OPENGL_MIN_MAJOR 4
#define VGFW_RENDER_API_OPENGL_MIN_MINOR 6
#endif

#define VGFW_TRACE(...) ::vgfw::log::g_Logger->trace(__VA_ARGS__)
#define VGFW_INFO(...) ::vgfw::log::g_Logger->info(__VA_ARGS__)
#define VGFW_WARN(...) ::vgfw::log::g_Logger->warn(__VA_ARGS__)
#define VGFW_ERROR(...) ::vgfw::log::g_Logger->error(__VA_ARGS__)
#define VGFW_CRITICAL(...) ::vgfw::log::g_Logger->critical(__VA_ARGS__)

// clang-format off
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
// clang-format on

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <sstream>
#include <stdexcept>
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

#include <fg/Blackboard.hpp>
#include <fg/FrameGraph.hpp>
#include <fg/FrameGraphResource.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
// #define TINYGLTF_NOEXCEPTION // optional. disable exception handling.
#include <tiny_gltf.h>

namespace vgfw
{
    // fwd
    namespace renderer
    {
        class GraphicsContext;
    }

    namespace utils
    {
        template<typename T, typename... Rest>
        void hashCombine(std::size_t& seed, const T& v, const Rest&... rest);

        std::string readFileAllText(const std::filesystem::path& filePath);
    } // namespace utils

    namespace math
    {
        inline constexpr bool isPowerOf2(uint32_t v) { return v && !(v & (v - 1)); }
    } // namespace math

    namespace log
    {
        static std::shared_ptr<spdlog::logger> g_Logger = nullptr;

        void init();
        void shutdown();
    } // namespace log

    namespace window
    {
        enum class AASample
        {
            e1  = 1,
            e2  = 2,
            e4  = 4,
            e8  = 8,
            e16 = 16
        };

        struct WindowInitInfo
        {
            std::string title        = "VGFW Window";
            uint32_t    width        = 1024;
            uint32_t    height       = 768;
            bool        isResizable  = false;
            bool        isFullScreen = false;
            bool        enableVSync  = false;
            AASample    aaSample     = AASample::e1;
        };

        enum class WindowType
        {
            eGLFW = 0,
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
            virtual WindowType getType() = 0;

            /**
             * @brief Initialize the window
             *
             * @param windowInitInfo
             */
            virtual bool init(const WindowInitInfo& windowInitInfo) = 0;

            /**
             * @brief Called each frame
             *
             */
            virtual void onTick() = 0;

            /**
             * @brief Get the width of window
             *
             * @return uint32_t
             */
            virtual uint32_t getWidth() const = 0;

            /**
             * @brief Get the height of window
             *
             * @return uint32_t
             */
            virtual uint32_t getHeight() const = 0;

            virtual bool shouldClose() const = 0;
            virtual bool isMinimized() const = 0;

            virtual void makeCurrentContext() = 0;
            virtual void swapBuffers()        = 0;

            virtual void setHideCursor(bool hide) = 0;

            virtual void* getPlatformWindow() const = 0;
            virtual void* getNativeWindow() const   = 0;

        protected:
            friend class renderer::GraphicsContext;

            /**
             * @brief Cleanup resources and shutdown the window.
             *
             */
            virtual void shutdown() = 0;
        };

        class GLFWWindow final : public Window
        {
        public:
            virtual WindowType getType() override { return WindowType::eGLFW; }

            virtual bool init(const WindowInitInfo& initInfo) override;

            virtual void onTick() override;

            virtual uint32_t getWidth() const override { return m_Data.width; }

            virtual uint32_t getHeight() const override { return m_Data.height; }

            virtual bool shouldClose() const override;
            virtual bool isMinimized() const override;

            virtual void makeCurrentContext() override;
            virtual void swapBuffers() override;

            virtual void setHideCursor(bool hide) override;

            virtual void* getPlatformWindow() const override { return m_Window; }

            virtual void* getNativeWindow() const override
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
            virtual void shutdown() override;

        private:
            GLFWwindow* m_Window {nullptr};

            struct WindowData
            {
                std::string  title;
                unsigned int width {0}, height {0};
                GLFWWindow*  platformWindow {nullptr};
                bool         isMinimized {false};
            };

            WindowData m_Data;
        };

        std::shared_ptr<Window> create(const WindowInitInfo& windowInitInfo, WindowType type = WindowType::eGLFW);
    } // namespace window

    namespace renderer
    {
        class GraphicsContext
        {
        public:
            GraphicsContext() = default;

            void init(const std::shared_ptr<window::Window>& window);
            void shutdown();

            void        swapBuffers();
            static void setVSync(bool vsyncEnabled);

            bool isSupportDSA() const { return m_SupportDSA; }

            inline std::shared_ptr<window::Window> getWindow() const { return m_Window; }

        private:
            static int loadGl();
            static int getMinMajor();
            static int getMinMinor();

        protected:
            bool                            m_SupportDSA {false};
            std::shared_ptr<window::Window> m_Window {nullptr};
        };

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

            GLsizeiptr getSize() const;
            bool       isMapped() const;

        protected:
            Buffer(GLuint id, GLsizeiptr size);

            explicit operator GLuint() const;

        protected:
            GLuint     m_Id {GL_NONE};
            GLsizeiptr m_Size {0};
            void*      m_MappedMemory {nullptr};
        };

        enum class IndexType
        {
            eUnknown = 0,
            eUInt8   = 1,
            eUInt16  = 2,
            eUInt32  = 4
        };

        class IndexBuffer final : public Buffer
        {
            friend class RenderContext;

        public:
            IndexBuffer() = default;

            IndexType  getIndexType() const;
            GLsizeiptr getCapacity() const;

        private:
            IndexBuffer(Buffer, IndexType);

        private:
            IndexType m_IndexType {IndexType::eUnknown};
        };

        class VertexBuffer final : public Buffer
        {
            friend class RenderContext;

        public:
            VertexBuffer() = default;

            GLsizei    getStride() const;
            GLsizeiptr getCapacity() const;

        private:
            VertexBuffer(Buffer, GLsizei stride);

        private:
            GLsizei m_Stride {0};
        };

        struct VertexAttribute
        {
            enum class Type
            {
                eFloat = 0,
                eFloat2,
                eFloat3,
                eFloat4,

                eInt,
                eInt4,

                eUByte4_Norm,
            };
            Type    vertType;
            int32_t offset;
        };

        using VertexAttributes = std::map<int32_t, VertexAttribute>;

        enum class AttributeLocation : int32_t
        {
            ePosition = 0,
            eNormal_Color,
            eTexCoords,
            eTangent,
            eBitangent,
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

            std::size_t getHash() const;

            const VertexAttributes& getAttributes() const;
            bool                    contains(AttributeLocation) const;
            bool                    contains(std::initializer_list<AttributeLocation>) const;

            uint32_t getStride() const;

            class Builder final
            {
            public:
                Builder()                   = default;
                Builder(const Builder&)     = delete;
                Builder(Builder&&) noexcept = delete;
                ~Builder()                  = default;

                Builder& operator=(const Builder&)     = delete;
                Builder& operator=(Builder&&) noexcept = delete;

                Builder& setAttribute(AttributeLocation, const VertexAttribute&);

                std::shared_ptr<VertexFormat> build();
                std::shared_ptr<VertexFormat> buildDefault();

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

        int32_t getSize(VertexAttribute::Type type);

        using Builder = VertexFormat::Builder;

        // clang-format off
        struct Offset2D
        {
            int32_t x {0}, y {0};

            auto operator<=> (const Offset2D&) const = default;
        };

        struct Extent2D
        {
            uint32_t width {0}, height {0};

            auto operator<=> (const Extent2D&) const = default;
        };

        struct Rect2D
        {
            Offset2D offset;
            Extent2D extent;

            auto operator<=> (const Rect2D&) const = default;
        };
        // clang-format on

        enum class CompareOp : GLenum
        {
            eNever          = GL_NEVER,
            eLess           = GL_LESS,
            eEqual          = GL_EQUAL,
            eLessOrEqual    = GL_LEQUAL,
            eGreater        = GL_GREATER,
            eNotEqual       = GL_NOTEQUAL,
            eGreaterOrEqual = GL_GEQUAL,
            eAlways         = GL_ALWAYS
        };

        using ViewportDesc = Rect2D;

        enum class PixelFormat : GLenum
        {
            eUnknown = GL_NONE,

            eR8_UNorm = GL_R8,
            eR32I     = GL_R32I,

            eRGB8_UNorm  = GL_RGB8,
            eRGBA8_UNorm = GL_RGBA8,

            eRGB8_SNorm  = GL_RGB8_SNORM,
            eRGBA8_SNorm = GL_RGBA8_SNORM,

            eR16F    = GL_R16F,
            eRG16F   = GL_RG16F,
            eRGB16F  = GL_RGB16F,
            eRGBA16F = GL_RGBA16F,

            eRGB32F = GL_RGB32F,

            eRGBA32F = GL_RGBA32F,

            eRGBA32UI = GL_RGBA32UI,

            eDepth16  = GL_DEPTH_COMPONENT16,
            eDepth24  = GL_DEPTH_COMPONENT24,
            eDepth32F = GL_DEPTH_COMPONENT32F
        };

        enum class TextureType : GLenum
        {
            eTexture2D = GL_TEXTURE_2D,
            eCubeMap   = GL_TEXTURE_CUBE_MAP
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

            GLenum getType() const;

            Extent2D    getExtent() const;
            uint32_t    getDepth() const;
            uint32_t    getNumMipLevels() const;
            uint32_t    getNumLayers() const;
            PixelFormat getPixelFormat() const;

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

            PixelFormat m_PixelFormat {PixelFormat::eUnknown};
        };

        enum class TexelFilter : GLenum
        {
            eNearest = GL_NEAREST,
            eLinear  = GL_LINEAR
        };
        enum class MipmapMode : GLenum
        {
            eNone    = GL_NONE,
            eNearest = GL_NEAREST,
            eLinear  = GL_LINEAR
        };

        enum class SamplerAddressMode : GLenum
        {
            eRepeat            = GL_REPEAT,
            eMirroredRepeat    = GL_MIRRORED_REPEAT,
            eClampToEdge       = GL_CLAMP_TO_EDGE,
            eClampToBorder     = GL_CLAMP_TO_BORDER,
            eMirrorClampToEdge = GL_MIRROR_CLAMP_TO_EDGE
        };

        struct SamplerInfo
        {
            TexelFilter minFilter {TexelFilter::eNearest};
            MipmapMode  mipmapMode {MipmapMode::eLinear};
            TexelFilter magFilter {TexelFilter::eLinear};

            SamplerAddressMode addressModeS {SamplerAddressMode::eRepeat};
            SamplerAddressMode addressModeT {SamplerAddressMode::eRepeat};
            SamplerAddressMode addressModeR {SamplerAddressMode::eRepeat};

            float maxAnisotropy {1.0f};

            std::optional<CompareOp> compareOperator {};
            glm::vec4                borderColor {0.0f};
        };

        uint32_t    calcMipLevels(uint32_t size);
        glm::uvec3  calcMipSize(const glm::uvec3& baseSize, uint32_t level);
        const char* toString(PixelFormat pixelFormat);

        struct DepthStencilState
        {
            bool      depthTest {false};
            bool      depthWrite {true};
            CompareOp depthCompareOp {CompareOp::eLess};

            // clang-format off
            auto operator<=> (const DepthStencilState&) const = default;
            // clang-format on
        };

        enum class BlendOp : GLenum
        {
            eAdd             = GL_FUNC_ADD,
            eSubtract        = GL_FUNC_SUBTRACT,
            eReverseSubtract = GL_FUNC_REVERSE_SUBTRACT,
            eMin             = GL_MIN,
            eMax             = GL_MAX
        };
        enum class BlendFactor : GLenum
        {
            eZero                  = GL_ZERO,
            eOne                   = GL_ONE,
            eSrcColor              = GL_SRC_COLOR,
            eOneMinusSrcColor      = GL_ONE_MINUS_SRC_COLOR,
            eDstColor              = GL_DST_COLOR,
            eOneMinusDstColor      = GL_ONE_MINUS_DST_COLOR,
            eSrcAlpha              = GL_SRC_ALPHA,
            eOneMinusSrcAlpha      = GL_ONE_MINUS_SRC_ALPHA,
            eDstAlpha              = GL_DST_ALPHA,
            eOneMinusDstAlpha      = GL_ONE_MINUS_DST_ALPHA,
            eConstantColor         = GL_CONSTANT_COLOR,
            eOneMinusConstantColor = GL_ONE_MINUS_CONSTANT_COLOR,
            eConstantAlpha         = GL_CONSTANT_ALPHA,
            eOneMinusConstantAlpha = GL_ONE_MINUS_CONSTANT_ALPHA,
            eSrcAlphaSaturate      = GL_SRC_ALPHA_SATURATE,
            eSrc1Color             = GL_SRC1_COLOR,
            eOneMinusSrc1Color     = GL_ONE_MINUS_SRC1_COLOR,
            eSrc1Alpha             = GL_SRC1_ALPHA,
            eOneMinusSrc1Alpha     = GL_ONE_MINUS_SRC1_ALPHA
        };

        // src = incoming values
        // dest = values that are already in a framebuffer
        struct BlendState
        {
            bool enabled {false};

            BlendFactor srcColor {BlendFactor::eOne};
            BlendFactor destColor {BlendFactor::eZero};
            BlendOp     colorOp {BlendOp::eAdd};

            BlendFactor srcAlpha {BlendFactor::eOne};
            BlendFactor destAlpha {BlendFactor::eZero};
            BlendOp     alphaOp {BlendOp::eAdd};

            // clang-format off
            auto operator<=> (const BlendState&) const = default;
            // clang-format on
        };
        constexpr auto kMaxNumBlendStates = 4;

        enum class PolygonMode : GLenum
        {
            ePoint = GL_POINT,
            eLine  = GL_LINE,
            eFill  = GL_FILL
        };
        enum class CullMode : GLenum
        {
            eNone  = GL_NONE,
            eBack  = GL_BACK,
            eFront = GL_FRONT
        };
        struct PolygonOffset
        {
            float factor {0.0f}, units {0.0f};

            // clang-format off
            auto operator<=> (const PolygonOffset&) const = default;
            // clang-format on
        };

        struct RasterizerState
        {
            PolygonMode                  polygonMode {PolygonMode::eFill};
            CullMode                     cullMode {CullMode::eBack};
            std::optional<PolygonOffset> polygonOffset;
            bool                         depthClampEnable {false};
            bool                         scissorTest {false};

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

                Builder& setShaderProgram(GLuint program);
                Builder& setVAO(GLuint vao);
                Builder& setDepthStencil(const DepthStencilState&);
                Builder& setRasterizerState(const RasterizerState&);
                Builder& setBlendState(uint32_t attachment, const BlendState&);

                GraphicsPipeline build();

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

        using UniformBuffer = Buffer;
        using StorageBuffer = Buffer;

        struct ImageData
        {
            GLenum      format {GL_NONE};
            GLenum      dataType {GL_NONE};
            const void* pixels {nullptr};
        };

        template<typename T>
        using OptionalReference = std::optional<std::reference_wrapper<T>>;

        using ClearValue = std::variant<glm::vec4, float>;

        struct AttachmentInfo
        {
            Texture&                  image;
            uint32_t                  mipLevel {0};
            std::optional<uint32_t>   layer {};
            std::optional<uint32_t>   face {};
            std::optional<ClearValue> clearValue {};
        };
        struct RenderingInfo
        {
            Rect2D                        area;
            std::vector<AttachmentInfo>   colorAttachments;
            std::optional<AttachmentInfo> depthAttachment {};
        };

        enum class PrimitiveTopology : GLenum
        {
            eUndefined = GL_NONE,

            ePointList = GL_POINTS,
            eLineList  = GL_LINES,
            eLineStrip = GL_LINE_STRIP,

            eTriangleList  = GL_TRIANGLES,
            eTriangleStrip = GL_TRIANGLE_STRIP,

            ePatchList = GL_PATCHES
        };

        class RenderContext
        {
        public:
            RenderContext();
            ~RenderContext();

            RenderContext& setViewport(const Rect2D& rect);
            static Rect2D  getViewport();

            RenderContext& setScissor(const Rect2D& rect);

            static Buffer       createBuffer(GLsizeiptr size, const void* data = nullptr);
            static VertexBuffer createVertexBuffer(GLsizei stride, int64_t capacity, const void* data = nullptr);
            static IndexBuffer  createIndexBuffer(IndexType, int64_t capacity, const void* data = nullptr);

            GLuint getVertexArray(const VertexAttributes&);

            static GLuint createGraphicsProgram(const std::string&                vertSource,
                                                const std::string&                fragSource,
                                                const std::optional<std::string>& geomSource = std::nullopt);

            static GLuint createComputeProgram(const std::string& compSource);

            static Texture
            createTexture2D(Extent2D extent, PixelFormat, uint32_t numMipLevels = 1u, uint32_t numLayers = 0u);
            static Texture createTexture3D(Extent2D, uint32_t depth, PixelFormat);
            static Texture
            createCubemap(uint32_t size, PixelFormat, uint32_t numMipLevels = 1u, uint32_t numLayers = 0u);

            RenderContext& generateMipmaps(Texture&);

            RenderContext& setupSampler(Texture&, const SamplerInfo&);
            static GLuint  createSampler(const SamplerInfo&);

            RenderContext& clear(Texture&);
            // Upload Texture2D
            RenderContext& upload(Texture&, GLint mipLevel, glm::uvec2 dimensions, const ImageData&);
            // Upload Cubemap face
            RenderContext& upload(Texture&, GLint mipLevel, GLint face, glm::uvec2 dimensions, const ImageData&);
            RenderContext&
            upload(Texture&, GLint mipLevel, const glm::uvec3& dimensions, GLint face, GLsizei layer, const ImageData&);

            RenderContext& clear(Buffer&);
            RenderContext& upload(Buffer&, GLintptr offset, GLsizeiptr size, const void* data);
            static void*   map(Buffer&);
            RenderContext& unmap(Buffer&);

            RenderContext& destroy(Buffer&);
            RenderContext& destroy(Texture&);
            RenderContext& destroy(GraphicsPipeline&);

            RenderContext& dispatch(GLuint computeProgram, const glm::uvec3& numGroups);

            GLuint         beginRendering(const RenderingInfo& info);
            RenderContext& beginRendering(const Rect2D&            area,
                                          std::optional<glm::vec4> clearColor   = {},
                                          std::optional<float>     clearDepth   = {},
                                          std::optional<int>       clearStencil = {});
            RenderContext& endRendering(GLuint frameBufferID);

            RenderContext& setUniform1f(const std::string& name, float);
            RenderContext& setUniform1i(const std::string& name, int32_t);
            RenderContext& setUniform1ui(const std::string& name, uint32_t);

            RenderContext& setUniformVec3(const std::string& name, const glm::vec3&);
            RenderContext& setUniformVec4(const std::string& name, const glm::vec4&);

            RenderContext& setUniformMat3(const std::string& name, const glm::mat3&);
            RenderContext& setUniformMat4(const std::string& name, const glm::mat4&);

            RenderContext& bindGraphicsPipeline(const GraphicsPipeline& gp);
            RenderContext& bindImage(GLuint unit, const Texture&, GLint mipLevel, GLenum access);
            RenderContext& bindTexture(GLuint unit, const Texture&, std::optional<GLuint> samplerId = {});
            RenderContext& bindUniformBuffer(GLuint index, const UniformBuffer&);
            RenderContext& bindStorageBuffer(GLuint index, const StorageBuffer&);

            RenderContext& drawFullScreenTriangle();
            RenderContext& drawCube();
            RenderContext& draw(OptionalReference<const VertexBuffer> vertexBuffer,
                                OptionalReference<const IndexBuffer>  indexBuffer,
                                uint32_t                              numIndices,
                                uint32_t                              numVertices,
                                uint32_t                              numInstances = 1);

            struct ResourceDeleter
            {
                void operator()(auto* ptr)
                {
                    context.destroy(*ptr);
                    delete ptr;
                }

                RenderContext& context;
            };

        private:
            static GLuint createVertexArray(const VertexAttributes&);

            static Texture createImmutableTexture(Extent2D,
                                                  uint32_t depth,
                                                  PixelFormat,
                                                  uint32_t numFaces,
                                                  uint32_t numMipLevels,
                                                  uint32_t numLayers);

            static void createFaceView(Texture& cubeMap, GLuint mipLevel, GLuint layer, GLuint face);
            static void attachTexture(GLuint framebuffer, GLenum attachment, const AttachmentInfo&);

            static GLuint createShaderProgram(std::initializer_list<GLuint> shaders);
            static GLuint createShaderObject(GLenum type, const std::string& shaderSource);

            void setShaderProgram(GLuint);
            void setVertexArray(GLuint);
            void setVertexBuffer(const VertexBuffer&) const;
            void setIndexBuffer(const IndexBuffer&) const;

            void setDepthTest(bool enabled, CompareOp);
            void setDepthWrite(bool enabled);

            void setPolygonMode(PolygonMode);
            void setPolygonOffset(std::optional<PolygonOffset>);
            void setCullMode(CullMode);
            void setDepthClamp(bool enabled);
            void setScissorTest(bool enabled);

            void setBlendState(GLuint index, const BlendState&);

        private:
            bool             m_RenderingStarted = false;
            GraphicsPipeline m_CurrentPipeline;

            GLuint                                  m_DummyVAO {GL_NONE};
            std::unordered_map<std::size_t, GLuint> m_VertexArrays;
        };

        // @return {data type, number of components, normalize}
        std::tuple<GLenum, GLint, GLboolean> statAttribute(VertexAttribute::Type type);
        GLenum                               selectTextureMinFilter(TexelFilter minFilter, MipmapMode mipmapMode);
        GLenum                               getIndexDataType(GLsizei stride);
        GLenum                               getPolygonOffsetCap(PolygonMode polygonMode);

        namespace framegraph
        {
            class FrameGraphBuffer
            {
            public:
                struct Desc
                {
                    GLsizeiptr size;
                };

                // NOLINTBEGIN
                void create(const Desc&, void* allocator);
                void destroy(const Desc&, void* allocator) const;
                // NOLINTEND

                Buffer* handle = nullptr;
            };

            enum class WrapMode
            {
                eClampToEdge = 0,
                eClampToOpaqueBlack,
                eClampToOpaqueWhite
            };

            class FrameGraphTexture
            {
            public:
                struct Desc
                {
                    Extent2D    extent;
                    uint32_t    depth {0};
                    uint32_t    numMipLevels {1};
                    uint32_t    layers {0};
                    PixelFormat format {PixelFormat::eUnknown};

                    bool        shadowSampler {false};
                    WrapMode    wrap {WrapMode::eClampToEdge};
                    TexelFilter filter {TexelFilter::eLinear};
                };

                // NOLINTBEGIN
                void create(const Desc& desc, void* allocator);
                void destroy(const Desc& desc, void* allocator) const;
                // NOLINTEND

                Texture* handle = nullptr;
            };

            class TransientResources
            {
            public:
                TransientResources() = delete;
                explicit TransientResources(RenderContext&);
                TransientResources(const TransientResources&)     = delete;
                TransientResources(TransientResources&&) noexcept = delete;
                ~TransientResources();

                TransientResources& operator=(const TransientResources&)     = delete;
                TransientResources& operator=(TransientResources&&) noexcept = delete;

                void update(float dt);

                Texture* acquireTexture(const FrameGraphTexture::Desc&);
                void     releaseTexture(const FrameGraphTexture::Desc&, Texture*);

                Buffer* acquireBuffer(const FrameGraphBuffer::Desc&);
                void    releaseBuffer(const FrameGraphBuffer::Desc&, Buffer*);

            private:
                RenderContext& m_RenderContext;

                std::vector<std::unique_ptr<Texture>> m_Textures;
                std::vector<std::unique_ptr<Buffer>>  m_Buffers;

                template<typename T>
                struct ResourceEntry
                {
                    T     resource;
                    float life;
                };
                template<typename T>
                using ResourcePool = std::vector<ResourceEntry<T>>;

                std::unordered_map<std::size_t, ResourcePool<Texture*>> m_TexturePools;
                std::unordered_map<std::size_t, ResourcePool<Buffer*>>  m_BufferPools;
            };

            FrameGraphResource importTexture(FrameGraph& fg, const std::string& name, Texture* texture);
            Texture&           getTexture(FrameGraphPassResources& resources, FrameGraphResource id);

            FrameGraphResource importBuffer(FrameGraph& fg, const std::string& name, Buffer* buffer);
            Buffer&            getBuffer(FrameGraphPassResources& resources, FrameGraphResource id);
        } // namespace framegraph

        namespace imgui
        {
            static bool g_EnableDocking = false;

            void init(bool enableDocking);
            void beginFrame();
            void endFrame();
            void shutdown();
        } // namespace imgui

        static bool                           g_RendererInit = false;
        static GraphicsContext                g_GraphicsContext;
        static std::shared_ptr<RenderContext> g_RenderContext = nullptr;

        struct RendererInitInfo
        {
            std::shared_ptr<window::Window> window {nullptr};
            bool                            enableImGuiDocking {false};
        };

        void init(const RendererInitInfo& initInfo);
        void beginImGui();
        void endImGui();
        void present();
        void shutdown();
        bool isLoaded();

        GraphicsContext& getGraphicsContext();
        RenderContext&   getRenderContext();
    } // namespace renderer

    namespace resource
    {
        struct Material
        {
            int baseColorTextureIndex {-1};
            int metallicRoughnessTextureIndex {-1};

            int normalTextureIndex {-1};
            int occlusionTextureIndex {-1};
            int emissiveTextureIndex {-1};
        };

        using PrimitiveMaterial = Material;

        struct MeshRecord
        {
            std::vector<glm::vec3> positions;
            std::vector<glm::vec3> normals;
            std::vector<glm::vec2> texcoords;
            std::vector<glm::vec4> tangents;
        };

        struct MeshPrimitive
        {
            std::string name;

            uint32_t indexCount {0};
            uint32_t vertexCount {0};

            MeshRecord record {};

            std::vector<uint32_t> indices;
            std::vector<float>    vertices;

            int materialIndex {-1};

            std::shared_ptr<renderer::VertexFormat> vertexFormat {nullptr};

            std::shared_ptr<renderer::IndexBuffer>  indexBuffer {nullptr};
            std::shared_ptr<renderer::VertexBuffer> vertexBuffer {nullptr};

            PrimitiveMaterial                 material {};
            std::shared_ptr<renderer::Buffer> materialBuffer {nullptr};
            std::vector<uint32_t>             textureIndices;

            void build(renderer::VertexFormat::Builder& vertexFormatBuilder, renderer::RenderContext& rc);
            void draw(renderer::RenderContext& rc) const;
        };

        struct Model
        {
            std::vector<MeshPrimitive> meshPrimitives;

            std::vector<vgfw::renderer::Texture*> textures;
            std::vector<Material>                 materials;

            void bindMeshPrimitiveTextures(uint32_t                 primitiveIndex,
                                           uint32_t                 unit,
                                           renderer::RenderContext& rc,
                                           std::optional<GLuint>    samplerId = {}) const;
        };
    } // namespace resource

    namespace io
    {
        static std::unordered_map<size_t, renderer::Texture*> g_TextureCache;

        renderer::Texture*
        load(const std::filesystem::path& texturePath, renderer::RenderContext& rc, bool flip = true);

        void release(const std::filesystem::path& texturePath, renderer::Texture& texture, renderer::RenderContext& rc);

        bool load(const std::filesystem::path& modelPath, resource::Model& model, renderer::RenderContext& rc);
    } // namespace io

    bool init();
    void shutdown();
} // namespace vgfw

// -------- hash specialization --------
namespace std
{
    template<>
    struct hash<vgfw::renderer::VertexAttribute>
    {
        std::size_t operator()(const vgfw::renderer::VertexAttribute& attribute) const noexcept
        {
            std::size_t h {0};
            vgfw::utils::hashCombine(h, attribute.vertType, attribute.offset);
            return h;
        }
    };

    template<>
    struct hash<vgfw::renderer::framegraph::FrameGraphTexture::Desc>
    {
        std::size_t operator()(const vgfw::renderer::framegraph::FrameGraphTexture::Desc& desc) const noexcept
        {
            std::size_t h {0};
            vgfw::utils::hashCombine(h,
                                     desc.extent.width,
                                     desc.extent.height,
                                     desc.depth,
                                     desc.numMipLevels,
                                     desc.layers,
                                     desc.format,
                                     desc.shadowSampler,
                                     desc.wrap,
                                     desc.filter);
            return h;
        }
    };
    template<>
    struct hash<vgfw::renderer::framegraph::FrameGraphBuffer::Desc>
    {
        std::size_t operator()(const vgfw::renderer::framegraph::FrameGraphBuffer::Desc& desc) const noexcept
        {
            std::size_t h {0};
            vgfw::utils::hashCombine(h, desc.size);
            return h;
        }
    };
} // namespace std

// -------- implementation --------
namespace vgfw
{
    namespace utils
    {
        template<typename T, typename... Rest>
        void hashCombine(std::size_t& seed, const T& v, const Rest&... rest)
        {
            // https://stackoverflow.com/questions/2590677/how-do-i-combine-hash-values-in-c0x
            seed ^= std::hash<T> {}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            (hashCombine(seed, rest), ...);
        }

        std::string readFileAllText(const std::filesystem::path& filePath)
        {
            std::ifstream fileStream(filePath);

            if (!fileStream.is_open())
            {
                throw std::runtime_error("Could not open file: " + filePath.string());
            }

            std::stringstream buffer;
            buffer << fileStream.rdbuf();

            return buffer.str();
        }
    } // namespace utils

    namespace log
    {
        void init()
        {
            std::vector<spdlog::sink_ptr> logSinks;

            logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
            logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("VGFW.log", true));

            logSinks[0]->set_pattern("%^[%T] %n: %v%$");
            logSinks[1]->set_pattern("[%T] [%l] %n: %v");

            g_Logger = std::make_shared<spdlog::logger>("VGFW", begin(logSinks), end(logSinks));
            spdlog::register_logger(g_Logger);
            g_Logger->set_level(spdlog::level::trace);
            g_Logger->flush_on(spdlog::level::trace);

            VGFW_INFO("[Logger] Initialized");
        }

        void shutdown()
        {
            VGFW_INFO("[Logger] Shutdown...");
            spdlog::shutdown();
        }
    } // namespace log

    namespace window
    {
        bool GLFWWindow::init(const WindowInitInfo& initInfo)
        {
            if (!glfwInit())
            {
                VGFW_ERROR("Failed to initialize GLFW");
                return false;
            }

            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, VGFW_RENDER_API_OPENGL_MIN_MAJOR);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, VGFW_RENDER_API_OPENGL_MIN_MINOR);

            glfwSwapInterval(initInfo.enableVSync);
            glfwWindowHint(GLFW_SAMPLES, static_cast<int>(initInfo.aaSample));
            glfwWindowHint(GLFW_RESIZABLE, initInfo.isResizable);

            GLFWmonitor*       primaryMonitor = glfwGetPrimaryMonitor();
            const GLFWvidmode* mode           = glfwGetVideoMode(primaryMonitor);

            GLFWmonitor* requestMonitor = nullptr;
            int          requestWidth   = initInfo.width;
            int          requestHeight  = initInfo.height;

            if (initInfo.isFullScreen)
            {
                requestMonitor = primaryMonitor;
                requestWidth   = mode->width;
                requestHeight  = mode->height;
            }

            m_Window = glfwCreateWindow(requestWidth, requestHeight, initInfo.title.c_str(), requestMonitor, nullptr);
            if (!m_Window)
            {
                VGFW_ERROR("Failed to create GLFW window");
                glfwTerminate();
                return false;
            }

            m_Data.title          = initInfo.title;
            m_Data.width          = initInfo.width;
            m_Data.height         = initInfo.height;
            m_Data.platformWindow = this;

            return true;
        }

        void GLFWWindow::shutdown()
        {
            if (m_Window)
            {
                glfwDestroyWindow(m_Window);
                m_Window = nullptr;
            }

            glfwTerminate();
        }

        void GLFWWindow::onTick() { glfwPollEvents(); }

        bool GLFWWindow::shouldClose() const { return m_Window && glfwWindowShouldClose(m_Window); }

        bool GLFWWindow::isMinimized() const
        {
            if (m_Window)
            {
                int width, height;
                glfwGetWindowSize(m_Window, &width, &height);
                return width == 0 || height == 0;
            }

            return false;
        }

        void GLFWWindow::makeCurrentContext()
        {
            if (m_Window)
            {
                glfwMakeContextCurrent(m_Window);
            }
        }

        void GLFWWindow::swapBuffers()
        {
            if (m_Window)
            {
                glfwSwapBuffers(m_Window);
            }
        }

        void GLFWWindow::setHideCursor(bool hide)
        {
            if (m_Window)
            {
                glfwSetInputMode(m_Window, GLFW_CURSOR, hide ? GLFW_CURSOR_HIDDEN : GLFW_CURSOR_NORMAL);
            }
        }

        std::shared_ptr<Window> create(const WindowInitInfo& windowInitInfo, WindowType type)
        {
            std::shared_ptr<Window> window = nullptr;

            switch (type)
            {
                case WindowType::eGLFW:
                    window = std::make_shared<GLFWWindow>();
                    break;
            }

            if (!window || !window->init(windowInitInfo))
            {
                throw std::runtime_error("Failed to initialize window!");
            }

            return window;
        }
    } // namespace window

    namespace renderer
    {
        void GraphicsContext::init(const std::shared_ptr<window::Window>& window)
        {
            m_Window = window;

            window->makeCurrentContext();
            int version = loadGl();

            assert(version);
            if (version)
            {
                int minMajor = getMinMajor();
                int minMinor = getMinMinor();
                VGFW_INFO("[OpenGLContext] Loaded {0}.{1}", GLVersion.major, GLVersion.minor);

                std::stringstream ss;
                ss << "    Vendor:       " << glGetString(GL_VENDOR) << std::endl;
                ss << "    Version:      " << glGetString(GL_VERSION) << std::endl;
                ss << "    Renderer:     " << glGetString(GL_RENDERER) << std::endl;
                ss << "    GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

                VGFW_INFO("[OpenGLContext] GL Context Info:\n {0}", ss.str());

                assert(GLVersion.major > minMajor || (GLVersion.major == minMajor && GLVersion.minor >= minMinor));
            }

            m_SupportDSA = GLAD_GL_VERSION_4_5 || GLAD_GL_VERSION_4_6;
        }

        void GraphicsContext::shutdown() { m_Window->shutdown(); }

        void GraphicsContext::swapBuffers() { m_Window->swapBuffers(); }

        void GraphicsContext::setVSync(bool vsyncEnabled) { glfwSwapInterval(vsyncEnabled); }

        int GraphicsContext::loadGl() { return gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)); }

        int GraphicsContext::getMinMajor() { return VGFW_RENDER_API_OPENGL_MIN_MAJOR; }

        int GraphicsContext::getMinMinor() { return VGFW_RENDER_API_OPENGL_MIN_MINOR; }

        Buffer::Buffer(Buffer&& other) noexcept :
            m_Id(other.m_Id), m_Size(other.m_Size), m_MappedMemory(other.m_MappedMemory)
        {
            memset(reinterpret_cast<void*>(&other), 0, sizeof(Buffer));
        }

        Buffer::~Buffer()
        {
            if (m_Id != GL_NONE)
                VGFW_ERROR("Buffer leak: {0}", m_Id);
        }

        Buffer& Buffer::operator=(Buffer&& rhs) noexcept
        {
            if (this != &rhs)
            {
                memcpy(reinterpret_cast<void*>(this), reinterpret_cast<void*>(&rhs), sizeof(Buffer));
                memset(reinterpret_cast<void*>(&rhs), 0, sizeof(Buffer));
            }
            return *this;
        }

        Buffer::operator bool() const { return m_Id != GL_NONE; }

        GLsizeiptr Buffer::getSize() const { return m_Size; }
        bool       Buffer::isMapped() const { return m_MappedMemory != nullptr; }

        Buffer::Buffer(GLuint id, GLsizeiptr size) : m_Id(id), m_Size(size) {}

        Buffer::operator GLuint() const { return m_Id; }

        IndexType  IndexBuffer::getIndexType() const { return m_IndexType; }
        GLsizeiptr IndexBuffer::getCapacity() const { return m_Size / static_cast<GLsizei>(m_IndexType); }

        IndexBuffer::IndexBuffer(Buffer buffer, IndexType indexType) :
            Buffer {std::move(buffer)}, m_IndexType {indexType}
        {}

        GLsizei    VertexBuffer::getStride() const { return m_Stride; }
        GLsizeiptr VertexBuffer::getCapacity() const { return m_Size / m_Stride; }

        VertexBuffer::VertexBuffer(Buffer buffer, GLsizei stride) : Buffer {std::move(buffer)}, m_Stride {stride} {}

        int32_t getSize(VertexAttribute::Type type)
        {
            switch (type)
            {
                using enum VertexAttribute::Type;
                case eFloat:
                    return sizeof(float);
                case eFloat2:
                    return sizeof(float) * 2;
                case eFloat3:
                    return sizeof(float) * 3;
                case eFloat4:
                    return sizeof(float) * 4;

                case VertexAttribute::Type::eInt:
                    return sizeof(int32_t);
                case eInt4:
                    return sizeof(int32_t) * 4;

                case eUByte4_Norm:
                    return sizeof(uint8_t) * 4;
            }
            return 0;
        }

        std::size_t VertexFormat::getHash() const { return m_Hash; }

        const VertexAttributes& VertexFormat::getAttributes() const { return m_Attributes; }
        bool                    VertexFormat::contains(AttributeLocation location) const
        {
            return m_Attributes.contains(static_cast<int32_t>(location));
        }

        bool VertexFormat::contains(std::initializer_list<AttributeLocation> locations) const
        {
            return std::all_of(std::cbegin(locations), std::cend(locations), [&](auto location) {
                return std::any_of(m_Attributes.cbegin(),
                                   m_Attributes.cend(),
                                   [v = static_cast<int32_t>(location)](const auto& p) { return p.first == v; });
            });
        }

        uint32_t VertexFormat::getStride() const { return m_Stride; }

        VertexFormat::VertexFormat(std::size_t hash, VertexAttributes&& attributes, uint32_t stride) :
            m_Hash {hash}, m_Attributes {attributes}, m_Stride {stride}
        {}

        using Builder = VertexFormat::Builder;

        Builder& Builder::setAttribute(AttributeLocation location, const VertexAttribute& attribute)
        {
            m_Attributes.insert_or_assign(static_cast<int32_t>(location), attribute);
            return *this;
        }

        std::shared_ptr<VertexFormat> Builder::build()
        {
            uint32_t    stride {0};
            std::size_t hash {0};
            for (const auto& [location, attribute] : m_Attributes)
            {
                stride += getSize(attribute.vertType);
                utils::hashCombine(hash, location, attribute);
            }

            if (const auto it = s_Cache.find(hash); it != s_Cache.cend())
                if (auto vertexFormat = it->second.lock(); vertexFormat)
                    return vertexFormat;

            auto vertexFormat = std::make_shared<VertexFormat>(VertexFormat {hash, std::move(m_Attributes), stride});
            s_Cache.insert_or_assign(hash, vertexFormat);
            return vertexFormat;
        }

        std::shared_ptr<VertexFormat> Builder::buildDefault()
        {
            m_Attributes.clear();

            setAttribute(AttributeLocation::ePosition, {.vertType = VertexAttribute::Type::eFloat3, .offset = 0});
            setAttribute(AttributeLocation::eNormal_Color, {.vertType = VertexAttribute::Type::eFloat3, .offset = 12});
            setAttribute(AttributeLocation::eTexCoords, {.vertType = VertexAttribute::Type::eFloat2, .offset = 24});

            return build();
        }

        Texture::Texture(Texture&& other) noexcept :
            m_Id {other.m_Id}, m_Type {other.m_Type}, m_View {other.m_View}, m_Extent {other.m_Extent},
            m_Depth {other.m_Depth}, m_NumMipLevels {other.m_NumMipLevels}, m_NumLayers {other.m_NumLayers},
            m_PixelFormat {other.m_PixelFormat}
        {
            memset(reinterpret_cast<void*>(&other), 0, sizeof(Texture));
        }

        Texture::~Texture()
        {
            if (m_Id != GL_NONE)
                VGFW_ERROR("Texture leak: {0}", m_Id);
        }

        Texture& Texture::operator=(Texture&& rhs) noexcept
        {
            if (this != &rhs)
            {
                memcpy(reinterpret_cast<void*>(this), reinterpret_cast<void*>(&rhs), sizeof(Texture));
                memset(reinterpret_cast<void*>(&rhs), 0, sizeof(Texture));
            }
            return *this;
        }

        Texture::operator bool() const { return m_Id != GL_NONE; }

        GLenum Texture::getType() const { return m_Type; }

        Extent2D    Texture::getExtent() const { return m_Extent; }
        uint32_t    Texture::getDepth() const { return m_Depth; }
        uint32_t    Texture::getNumMipLevels() const { return m_NumMipLevels; }
        uint32_t    Texture::getNumLayers() const { return m_NumLayers; }
        PixelFormat Texture::getPixelFormat() const { return m_PixelFormat; }

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

                case eR8_UNorm:
                    return "R8_Unorm";

                case eRGB8_UNorm:
                    return "RGB8_UNorm";
                case eRGBA8_UNorm:
                    return "RGBA8_UNorm";

                case eRGB8_SNorm:
                    return "RGB8_SNorm";
                case eRGBA8_SNorm:
                    return "RGBA8_SNorm";

                case eR16F:
                    return "R16F";
                case eRG16F:
                    return "RG16F";
                case eRGB16F:
                    return "RGB16F";
                case eRGBA16F:
                    return "RGBA16F";

                case eRGB32F:
                    return "RGB32F";
                case eRGBA32F:
                    return "RGBA32F";

                case eR32I:
                    return "R32I";
                case eRGBA32UI:
                    return "RGBA32UI";

                case eDepth16:
                    return "Depth16";
                case eDepth24:
                    return "Depth24";
                case eDepth32F:
                    return "Depth32F";

                case PixelFormat::eUnknown:
                    break;
            }

            return "Undefined";
        }

        GraphicsPipeline::Builder& GraphicsPipeline::Builder::setShaderProgram(GLuint program)
        {
            m_Program = program;
            return *this;
        }

        GraphicsPipeline::Builder& GraphicsPipeline::Builder::setVAO(GLuint vao)
        {
            m_VAO = vao;
            return *this;
        }

        GraphicsPipeline::Builder& GraphicsPipeline::Builder::setDepthStencil(const DepthStencilState& state)
        {
            m_DepthStencilState = state;
            return *this;
        }

        GraphicsPipeline::Builder& GraphicsPipeline::Builder::setRasterizerState(const RasterizerState& state)
        {
            m_RasterizerState = state;
            return *this;
        }

        GraphicsPipeline::Builder& GraphicsPipeline::Builder::setBlendState(uint32_t          attachment,
                                                                            const BlendState& state)
        {
            assert(attachment < kMaxNumBlendStates);
            m_BlendStates[attachment] = state;
            return *this;
        }

        GraphicsPipeline GraphicsPipeline::Builder::build()
        {
            GraphicsPipeline g;

            g.m_Program           = m_Program;
            g.m_VAO               = m_VAO;
            g.m_DepthStencilState = m_DepthStencilState;
            g.m_RasterizerState   = m_RasterizerState;
            g.m_BlendStates       = m_BlendStates;

            return g;
        }

        // @return {data type, number of components, normalize}
        std::tuple<GLenum, GLint, GLboolean> statAttribute(VertexAttribute::Type type)
        {
            switch (type)
            {
                using enum VertexAttribute::Type;
                case eFloat:
                    return {GL_FLOAT, 1, GL_FALSE};
                case eFloat2:
                    return {GL_FLOAT, 2, GL_FALSE};
                case eFloat3:
                    return {GL_FLOAT, 3, GL_FALSE};
                case eFloat4:
                    return {GL_FLOAT, 4, GL_FALSE};

                case eInt:
                    return {GL_INT, 1, GL_FALSE};
                case eInt4:
                    return {GL_INT, 4, GL_FALSE};

                case eUByte4_Norm:
                    return {GL_UNSIGNED_BYTE, 4, GL_TRUE};
            }
            return {GL_INVALID_INDEX, 0, GL_FALSE};
        }

        GLenum selectTextureMinFilter(TexelFilter minFilter, MipmapMode mipmapMode)
        {
            GLenum result {GL_NONE};
            switch (minFilter)
            {
                case TexelFilter::eNearest:
                    switch (mipmapMode)
                    {
                        case MipmapMode::eNone:
                            result = GL_NEAREST;
                            break;
                        case MipmapMode::eNearest:
                            result = GL_NEAREST_MIPMAP_NEAREST;
                            break;
                        case MipmapMode::eLinear:
                            result = GL_NEAREST_MIPMAP_LINEAR;
                            break;
                    }
                    break;

                case TexelFilter::eLinear:
                    switch (mipmapMode)
                    {
                        case MipmapMode::eNone:
                            result = GL_LINEAR;
                            break;
                        case MipmapMode::eNearest:
                            result = GL_LINEAR_MIPMAP_NEAREST;
                            break;
                        case MipmapMode::eLinear:
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
                case PolygonMode::eFill:
                    return GL_POLYGON_OFFSET_FILL;
                case PolygonMode::eLine:
                    return GL_POLYGON_OFFSET_LINE;
                case PolygonMode::ePoint:
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

        RenderContext& RenderContext::setViewport(const Rect2D& rect)
        {
            auto& current = m_CurrentPipeline.m_Viewport;
            if (rect != current)
            {
                glViewport(rect.offset.x, rect.offset.y, rect.extent.width, rect.extent.height);
                current = rect;
            }
            return *this;
        }

        Rect2D RenderContext::getViewport()
        {
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);

            return {{viewport[0], viewport[1]},
                    {static_cast<uint32_t>(viewport[2]), static_cast<uint32_t>(viewport[3])}};
        }

        RenderContext& RenderContext::setScissor(const Rect2D& rect)
        {
            auto& current = m_CurrentPipeline.m_Scissor;
            if (rect != current)
            {
                glScissor(rect.offset.x, rect.offset.y, rect.extent.width, rect.extent.height);
                current = rect;
            }
            return *this;
        }

        Buffer RenderContext::createBuffer(GLsizeiptr size, const void* data)
        {
            GLuint buffer;
            glCreateBuffers(1, &buffer);
            glNamedBufferStorage(buffer, size, data, GL_DYNAMIC_STORAGE_BIT);

            return {buffer, size};
        }

        VertexBuffer RenderContext::createVertexBuffer(GLsizei stride, int64_t capacity, const void* data)
        {
            return VertexBuffer {createBuffer(stride * capacity, data), stride};
        }

        IndexBuffer RenderContext::createIndexBuffer(IndexType indexType, int64_t capacity, const void* data)
        {
            const auto stride = static_cast<GLsizei>(indexType);
            return IndexBuffer {createBuffer(stride * capacity, data), indexType};
        }

        GLuint RenderContext::getVertexArray(const VertexAttributes& attributes)
        {
            assert(!attributes.empty());

            std::size_t hash {0};
            for (const auto& [location, attribute] : attributes)
                utils::hashCombine(hash, location, attribute);

            auto it = m_VertexArrays.find(hash);
            if (it == m_VertexArrays.cend())
            {
                it = m_VertexArrays.emplace(hash, createVertexArray(attributes)).first;
                VGFW_TRACE("[RenderContext] Created VAO: {0}", hash);
            }

            return it->second;
        }

        GLuint RenderContext::createGraphicsProgram(const std::string&                vertSource,
                                                    const std::string&                fragSource,
                                                    const std::optional<std::string>& geomSource)
        {
            return createShaderProgram({
                createShaderObject(GL_VERTEX_SHADER, vertSource),
                geomSource ? createShaderObject(GL_GEOMETRY_SHADER, *geomSource) : GL_NONE,
                createShaderObject(GL_FRAGMENT_SHADER, fragSource),
            });
        }

        GLuint RenderContext::createComputeProgram(const std::string& compSource)
        {
            return createShaderProgram({
                createShaderObject(GL_COMPUTE_SHADER, compSource),
            });
        }

        Texture RenderContext::createTexture2D(Extent2D    extent,
                                               PixelFormat pixelFormat,
                                               uint32_t    numMipLevels,
                                               uint32_t    numLayers)
        {
            assert(extent.width > 0 && extent.height > 0 && pixelFormat != PixelFormat::eUnknown);

            if (numMipLevels <= 0)
                numMipLevels = calcMipLevels(glm::max(extent.width, extent.height));

            return createImmutableTexture(extent, 0, pixelFormat, 1, numMipLevels, numLayers);
        }

        Texture RenderContext::createTexture3D(Extent2D extent, uint32_t depth, PixelFormat pixelFormat)
        {
            return createImmutableTexture(extent, depth, pixelFormat, 1, 1, 0);
        }

        Texture
        RenderContext::createCubemap(uint32_t size, PixelFormat pixelFormat, uint32_t numMipLevels, uint32_t numLayers)
        {
            assert(size > 0 && pixelFormat != PixelFormat::eUnknown);

            if (numMipLevels <= 0)
                numMipLevels = calcMipLevels(size);

            return createImmutableTexture({size, size}, 0, pixelFormat, 6, numMipLevels, numLayers);
        }

        RenderContext& RenderContext::generateMipmaps(Texture& texture)
        {
            assert(texture);
            glGenerateTextureMipmap(texture.m_Id);

            return *this;
        }

        RenderContext& RenderContext::setupSampler(Texture& texture, const SamplerInfo& samplerInfo)
        {
            assert(texture);

            glTextureParameteri(texture.m_Id,
                                GL_TEXTURE_MIN_FILTER,
                                selectTextureMinFilter(samplerInfo.minFilter, samplerInfo.mipmapMode));
            glTextureParameteri(texture.m_Id, GL_TEXTURE_MAG_FILTER, static_cast<GLenum>(samplerInfo.magFilter));
            glTextureParameteri(texture.m_Id, GL_TEXTURE_WRAP_S, static_cast<GLenum>(samplerInfo.addressModeS));
            glTextureParameteri(texture.m_Id, GL_TEXTURE_WRAP_T, static_cast<GLenum>(samplerInfo.addressModeT));
            glTextureParameteri(texture.m_Id, GL_TEXTURE_WRAP_R, static_cast<GLenum>(samplerInfo.addressModeR));

            glTextureParameterf(texture.m_Id, GL_TEXTURE_MAX_ANISOTROPY, samplerInfo.maxAnisotropy);

            if (samplerInfo.compareOperator.has_value())
            {
                glTextureParameteri(texture.m_Id, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                glTextureParameteri(
                    texture.m_Id, GL_TEXTURE_COMPARE_FUNC, static_cast<GLenum>(*samplerInfo.compareOperator));
            }
            glTextureParameterfv(texture.m_Id, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(samplerInfo.borderColor));

            return *this;
        }

        GLuint RenderContext::createSampler(const SamplerInfo& samplerInfo)
        {
            GLuint sampler {GL_NONE};
            glCreateSamplers(1, &sampler);

            glSamplerParameteri(
                sampler, GL_TEXTURE_MIN_FILTER, selectTextureMinFilter(samplerInfo.minFilter, samplerInfo.mipmapMode));
            glSamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, static_cast<GLenum>(samplerInfo.magFilter));
            glSamplerParameteri(sampler, GL_TEXTURE_WRAP_S, static_cast<GLenum>(samplerInfo.addressModeS));
            glSamplerParameteri(sampler, GL_TEXTURE_WRAP_T, static_cast<GLenum>(samplerInfo.addressModeT));
            glSamplerParameteri(sampler, GL_TEXTURE_WRAP_R, static_cast<GLenum>(samplerInfo.addressModeR));

            glSamplerParameterf(sampler, GL_TEXTURE_MAX_ANISOTROPY, samplerInfo.maxAnisotropy);

            if (samplerInfo.compareOperator.has_value())
            {
                glSamplerParameteri(sampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
                glSamplerParameteri(
                    sampler, GL_TEXTURE_COMPARE_FUNC, static_cast<GLenum>(*samplerInfo.compareOperator));
            }
            glSamplerParameterfv(sampler, GL_TEXTURE_BORDER_COLOR, glm::value_ptr(samplerInfo.borderColor));

            return sampler;
        }

        RenderContext& RenderContext::clear(Texture& texture)
        {
            assert(texture);
            uint8_t v {0};
            glClearTexImage(texture.m_Id, 0, GL_RED, GL_UNSIGNED_BYTE, &v);

            return *this;
        }

        RenderContext&
        RenderContext::upload(Texture& texture, GLint mipLevel, glm::uvec2 dimensions, const ImageData& image)
        {
            return upload(texture, mipLevel, {dimensions, 0}, 0, 0, image);
        }

        RenderContext& RenderContext::upload(Texture&         texture,
                                             GLint            mipLevel,
                                             GLint            face,
                                             glm::uvec2       dimensions,
                                             const ImageData& image)
        {
            return upload(texture, mipLevel, {dimensions, 0}, face, 0, image);
        }

        RenderContext& RenderContext::upload(Texture&          texture,
                                             GLint             mipLevel,
                                             const glm::uvec3& dimensions,
                                             GLint             face,
                                             GLsizei           layer,
                                             const ImageData&  image)
        {
            assert(texture && image.pixels != nullptr);

            switch (texture.m_Type)
            {
                case GL_TEXTURE_1D:
                    glTextureSubImage1D(
                        texture.m_Id, mipLevel, 0, dimensions.x, image.format, image.dataType, image.pixels);
                    break;
                case GL_TEXTURE_1D_ARRAY:
                    glTextureSubImage2D(
                        texture.m_Id, mipLevel, 0, 0, dimensions.x, layer, image.format, image.dataType, image.pixels);
                    break;
                case GL_TEXTURE_2D:
                    glTextureSubImage2D(texture.m_Id,
                                        mipLevel,
                                        0,
                                        0,
                                        dimensions.x,
                                        dimensions.y,
                                        image.format,
                                        image.dataType,
                                        image.pixels);
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
                                        image.format,
                                        image.dataType,
                                        image.pixels);
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
                                        image.format,
                                        image.dataType,
                                        image.pixels);
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
                                        image.format,
                                        image.dataType,
                                        image.pixels);
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
                                        image.format,
                                        image.dataType,
                                        image.pixels);
                }
                break;

                default:
                    assert(false);
            }
            return *this;
        }

        RenderContext& RenderContext::clear(Buffer& buffer)
        {
            assert(buffer);

            uint8_t v {0};
            glClearNamedBufferData(buffer.m_Id, GL_R8, GL_RED, GL_UNSIGNED_BYTE, &v);

            return *this;
        }

        RenderContext& RenderContext::upload(Buffer& buffer, GLintptr offset, GLsizeiptr size, const void* data)
        {
            assert(buffer);

            if (size > 0 && data != nullptr)
                glNamedBufferSubData(buffer.m_Id, offset, size, data);

            return *this;
        }

        void* RenderContext::map(Buffer& buffer)
        {
            assert(buffer);

            if (!buffer.isMapped())
                buffer.m_MappedMemory = glMapNamedBuffer(buffer.m_Id, GL_WRITE_ONLY);

            return buffer.m_MappedMemory;
        }

        RenderContext& RenderContext::unmap(Buffer& buffer)
        {
            assert(buffer);

            if (buffer.isMapped())
            {
                glUnmapNamedBuffer(buffer.m_Id);
                buffer.m_MappedMemory = nullptr;
            }

            return *this;
        }

        RenderContext& RenderContext::destroy(Buffer& buffer)
        {
            if (buffer)
            {
                glDeleteBuffers(1, &buffer.m_Id);
                buffer = {};
            }

            return *this;
        }

        RenderContext& RenderContext::destroy(Texture& texture)
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

        RenderContext& RenderContext::destroy(GraphicsPipeline& gp)
        {
            if (gp.m_Program != GL_NONE)
            {
                glDeleteProgram(gp.m_Program);
                gp.m_Program = GL_NONE;
            }
            gp.m_VAO = GL_NONE;

            return *this;
        }

        RenderContext& RenderContext::dispatch(GLuint computeProgram, const glm::uvec3& numGroups)
        {
            setShaderProgram(computeProgram);
            glDispatchCompute(numGroups.x, numGroups.y, numGroups.z);

            return *this;
        }

        GLuint RenderContext::beginRendering(const RenderingInfo& renderingInfo)
        {
            assert(!m_RenderingStarted);

            GLuint framebuffer;
            glCreateFramebuffers(1, &framebuffer);
            if (renderingInfo.depthAttachment.has_value())
            {
                attachTexture(framebuffer, GL_DEPTH_ATTACHMENT, *renderingInfo.depthAttachment);
            }
            for (size_t i {0}; i < renderingInfo.colorAttachments.size(); ++i)
            {
                attachTexture(framebuffer, GL_COLOR_ATTACHMENT0 + i, renderingInfo.colorAttachments[i]);
            }
            if (const auto n = renderingInfo.colorAttachments.size(); n > 0)
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
            setViewport(renderingInfo.area);
            setScissorTest(false);

            if (renderingInfo.depthAttachment.has_value())
                if (renderingInfo.depthAttachment->clearValue.has_value())
                {
                    setDepthWrite(true);

                    const auto clearValue = std::get<float>(*renderingInfo.depthAttachment->clearValue);
                    glClearNamedFramebufferfv(framebuffer, GL_DEPTH, 0, &clearValue);
                }
            for (int32_t i {0}; const auto& attachment : renderingInfo.colorAttachments)
            {
                if (attachment.clearValue.has_value())
                {
                    const auto& clearValue = std::get<glm::vec4>(*attachment.clearValue);
                    glClearNamedFramebufferfv(framebuffer, GL_COLOR, i, glm::value_ptr(clearValue));
                }
                ++i;
            }

            m_RenderingStarted = true;

            return framebuffer;
        }

        RenderContext& RenderContext::beginRendering(const Rect2D&            area,
                                                     std::optional<glm::vec4> clearColor,
                                                     std::optional<float>     clearDepth,
                                                     std::optional<int>       clearStencil)
        {
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GL_NONE);
            setViewport(area);
            setScissorTest(false);

            if (clearDepth.has_value())
            {
                setDepthWrite(true);
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

        RenderContext& RenderContext::endRendering(GLuint frameBufferID)
        {
            assert(m_RenderingStarted && frameBufferID != GL_NONE);

            glDeleteFramebuffers(1, &frameBufferID);
            m_RenderingStarted = false;

            return *this;
        }

        RenderContext& RenderContext::bindGraphicsPipeline(const GraphicsPipeline& gp)
        {
            {
                const auto& state = gp.m_DepthStencilState;
                setDepthTest(state.depthTest, state.depthCompareOp);
                setDepthWrite(state.depthWrite);
            }

            {
                const auto& state = gp.m_RasterizerState;
                setPolygonMode(state.polygonMode);
                setCullMode(state.cullMode);
                setPolygonOffset(state.polygonOffset);
                setDepthClamp(state.depthClampEnable);
                setScissorTest(state.scissorTest);
            }

            for (int32_t i {0}; i < gp.m_BlendStates.size(); ++i)
                setBlendState(i, gp.m_BlendStates[i]);

            setVertexArray(gp.m_VAO);
            setShaderProgram(gp.m_Program);

            return *this;
        }

        RenderContext& RenderContext::setUniform1f(const std::string& name, float f)
        {
            const auto location = glGetUniformLocation(m_CurrentPipeline.m_Program, name.data());
            if (location != GL_INVALID_INDEX)
                glProgramUniform1f(m_CurrentPipeline.m_Program, location, f);
            return *this;
        }

        RenderContext& RenderContext::setUniform1i(const std::string& name, int32_t i)
        {
            const auto location = glGetUniformLocation(m_CurrentPipeline.m_Program, name.data());
            if (location != GL_INVALID_INDEX)
                glProgramUniform1i(m_CurrentPipeline.m_Program, location, i);
            return *this;
        }

        RenderContext& RenderContext::setUniform1ui(const std::string& name, uint32_t i)
        {
            const auto location = glGetUniformLocation(m_CurrentPipeline.m_Program, name.data());
            if (location != GL_INVALID_INDEX)
                glProgramUniform1ui(m_CurrentPipeline.m_Program, location, i);
            return *this;
        }

        RenderContext& RenderContext::setUniformVec3(const std::string& name, const glm::vec3& v)
        {
            const auto location = glGetUniformLocation(m_CurrentPipeline.m_Program, name.data());
            if (location != GL_INVALID_INDEX)
            {
                glProgramUniform3fv(m_CurrentPipeline.m_Program, location, 1, glm::value_ptr(v));
            }
            return *this;
        }

        RenderContext& RenderContext::setUniformVec4(const std::string& name, const glm::vec4& v)
        {
            const auto location = glGetUniformLocation(m_CurrentPipeline.m_Program, name.data());
            if (location != GL_INVALID_INDEX)
            {
                glProgramUniform4fv(m_CurrentPipeline.m_Program, location, 1, glm::value_ptr(v));
            }
            return *this;
        }

        RenderContext& RenderContext::setUniformMat3(const std::string& name, const glm::mat3& m)
        {
            const auto location = glGetUniformLocation(m_CurrentPipeline.m_Program, name.data());
            if (location != GL_INVALID_INDEX)
            {
                glProgramUniformMatrix3fv(m_CurrentPipeline.m_Program, location, 1, GL_FALSE, glm::value_ptr(m));
            }
            return *this;
        }

        RenderContext& RenderContext::setUniformMat4(const std::string& name, const glm::mat4& m)
        {
            const auto location = glGetUniformLocation(m_CurrentPipeline.m_Program, name.data());
            if (location != GL_INVALID_INDEX)
            {
                glProgramUniformMatrix4fv(m_CurrentPipeline.m_Program, location, 1, GL_FALSE, glm::value_ptr(m));
            }
            return *this;
        }

        RenderContext& RenderContext::bindImage(GLuint unit, const Texture& texture, GLint mipLevel, GLenum access)
        {
            assert(texture && mipLevel < texture.m_NumMipLevels);
            glBindImageTexture(
                unit, texture.m_Id, mipLevel, GL_FALSE, 0, access, static_cast<GLenum>(texture.m_PixelFormat));
            return *this;
        }

        RenderContext& RenderContext::bindTexture(GLuint unit, const Texture& texture, std::optional<GLuint> samplerId)
        {
            assert(texture);
            glBindTextureUnit(unit, texture.m_Id);
            if (samplerId.has_value())
                glBindSampler(unit, *samplerId);
            return *this;
        }

        RenderContext& RenderContext::bindUniformBuffer(GLuint index, const UniformBuffer& buffer)
        {
            assert(buffer);
            glBindBufferBase(GL_UNIFORM_BUFFER, index, buffer.m_Id);
            return *this;
        }

        RenderContext& RenderContext::bindStorageBuffer(GLuint index, const StorageBuffer& buffer)
        {
            assert(buffer);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, index, buffer.m_Id);
            return *this;
        }

        RenderContext& RenderContext::drawFullScreenTriangle() { return draw({}, {}, 0, 3); }

        RenderContext& RenderContext::drawCube() { return draw({}, {}, 0, 36); }

        RenderContext& RenderContext::draw(OptionalReference<const VertexBuffer> vertexBuffer,
                                           OptionalReference<const IndexBuffer>  indexBuffer,
                                           uint32_t                              numIndices,
                                           uint32_t                              numVertices,
                                           uint32_t                              numInstances)
        {
            if (vertexBuffer.has_value())
                setVertexBuffer(*vertexBuffer);

            if (numIndices > 0)
            {
                assert(indexBuffer.has_value());
                setIndexBuffer(*indexBuffer);
                glDrawElementsInstanced(GL_TRIANGLES, numIndices, GL_UNSIGNED_INT, nullptr, numInstances);
            }
            else
            {
                glDrawArraysInstanced(GL_TRIANGLES, 0, numVertices, numInstances);
            }
            return *this;
        }

        GLuint RenderContext::createVertexArray(const VertexAttributes& attributes)
        {
            GLuint vao;
            glCreateVertexArrays(1, &vao);

            for (const auto& [location, attribute] : attributes)
            {
                const auto [type, size, normalized] = statAttribute(attribute.vertType);
                assert(type != GL_INVALID_INDEX);

                glEnableVertexArrayAttrib(vao, location);
                if (attribute.vertType == VertexAttribute::Type::eInt4)
                {
                    glVertexArrayAttribIFormat(vao, location, size, type, attribute.offset);
                }
                else
                {
                    glVertexArrayAttribFormat(vao, location, size, type, normalized, attribute.offset);
                }
                glVertexArrayAttribBinding(vao, location, 0);
            }
            return vao;
        }

        Texture RenderContext::createImmutableTexture(Extent2D    extent,
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
                            extent.height > 0 ? GL_TEXTURE_2D :
                                                GL_TEXTURE_1D;
            assert(target == GL_TEXTURE_CUBE_MAP ? extent.width == extent.height : true);

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
                    glTextureStorage1D(id, numMipLevels, internalFormat, extent.width);
                    break;
                case GL_TEXTURE_1D_ARRAY:
                    glTextureStorage2D(id, numMipLevels, internalFormat, extent.width, numLayers);
                    break;

                case GL_TEXTURE_2D:
                    glTextureStorage2D(id, numMipLevels, internalFormat, extent.width, extent.height);
                    break;
                case GL_TEXTURE_2D_ARRAY:
                    glTextureStorage3D(id, numMipLevels, internalFormat, extent.width, extent.height, numLayers);
                    break;

                case GL_TEXTURE_3D:
                    glTextureStorage3D(id, numMipLevels, internalFormat, extent.width, extent.height, depth);
                    break;

                case GL_TEXTURE_CUBE_MAP:
                    glTextureStorage2D(id, numMipLevels, internalFormat, extent.width, extent.height);
                    break;
                case GL_TEXTURE_CUBE_MAP_ARRAY:
                    glTextureStorage3D(id, numMipLevels, internalFormat, extent.width, extent.height, numLayers * 6);
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

        void RenderContext::createFaceView(Texture& cubeMap, GLuint mipLevel, GLuint layer, GLuint face)
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

        void RenderContext::attachTexture(GLuint framebuffer, GLenum attachment, const AttachmentInfo& info)
        {
            const auto& [image, mipLevel, maybeLayer, maybeFace, _] = info;

            switch (image.m_Type)
            {
                case GL_TEXTURE_CUBE_MAP:
                case GL_TEXTURE_CUBE_MAP_ARRAY:
                    createFaceView(image, mipLevel, maybeLayer.value_or(0), maybeFace.value_or(0));
                    glNamedFramebufferTexture(framebuffer, attachment, image.m_View, 0);
                    break;

                case GL_TEXTURE_2D:
                    glNamedFramebufferTexture(framebuffer, attachment, image.m_Id, mipLevel);
                    break;

                case GL_TEXTURE_2D_ARRAY:
                    assert(maybeLayer.has_value());
                    glNamedFramebufferTextureLayer(
                        framebuffer, attachment, image.m_Id, mipLevel, maybeLayer.value_or(0));
                    break;

                case GL_TEXTURE_3D:
                    glNamedFramebufferTexture(framebuffer, attachment, image.m_Id, 0);
                    break;

                default:
                    assert(false);
            }
        }

        GLuint RenderContext::createShaderProgram(std::initializer_list<GLuint> shaders)
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

                VGFW_ERROR("[ShaderInfoLog] {0}", infoLog);

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

        GLuint RenderContext::createShaderObject(GLenum type, const std::string& shaderSource)
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

                VGFW_ERROR("[ShaderInfoLog] {0}", infoLog);
                throw std::runtime_error {infoLog};
            };

            return id;
        }

        void RenderContext::setShaderProgram(GLuint program)
        {
            assert(program != GL_NONE);
            if (auto& current = m_CurrentPipeline.m_Program; current != program)
            {
                glUseProgram(program);
                current = program;
            }
        }

        void RenderContext::setVertexArray(GLuint vao)
        {
            if (GL_NONE == vao)
                vao = m_DummyVAO;

            if (auto& current = m_CurrentPipeline.m_VAO; vao != current)
            {
                glBindVertexArray(vao);
                current = vao;
            }
        }

        void RenderContext::setVertexBuffer(const VertexBuffer& vertexBuffer) const
        {
            const auto vao = m_CurrentPipeline.m_VAO;
            assert(vertexBuffer && vao != GL_NONE);
            glVertexArrayVertexBuffer(vao, 0, vertexBuffer.m_Id, 0, vertexBuffer.getStride());
        }

        void RenderContext::setIndexBuffer(const IndexBuffer& indexBuffer) const
        {
            const auto vao = m_CurrentPipeline.m_VAO;
            assert(indexBuffer && vao != GL_NONE);
            glVertexArrayElementBuffer(vao, indexBuffer.m_Id);
        }

        void RenderContext::setDepthTest(bool enabled, CompareOp depthFunc)
        {
            auto& current = m_CurrentPipeline.m_DepthStencilState;
            if (enabled != current.depthTest)
            {
                enabled ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST);
                current.depthTest = enabled;
            }
            if (enabled && depthFunc != current.depthCompareOp)
            {
                glDepthFunc(static_cast<GLenum>(depthFunc));
                current.depthCompareOp = depthFunc;
            }
        }

        void RenderContext::setDepthWrite(bool enabled)
        {
            auto& current = m_CurrentPipeline.m_DepthStencilState;
            if (enabled != current.depthWrite)
            {
                glDepthMask(enabled);
                current.depthWrite = enabled;
            }
        }

        void RenderContext::setPolygonMode(PolygonMode polygonMode)
        {
            auto& current = m_CurrentPipeline.m_RasterizerState.polygonMode;
            if (polygonMode != current)
            {
                glPolygonMode(GL_FRONT_AND_BACK, static_cast<GLenum>(polygonMode));
                current = polygonMode;
            }
        }

        void RenderContext::setPolygonOffset(std::optional<PolygonOffset> polygonOffset)
        {
            auto& current = m_CurrentPipeline.m_RasterizerState;
            if (polygonOffset != current.polygonOffset)
            {
                const auto offsetCap = getPolygonOffsetCap(current.polygonMode);
                if (polygonOffset.has_value())
                {
                    glEnable(offsetCap);
                    glPolygonOffset(polygonOffset->factor, polygonOffset->units);
                }
                else
                {
                    glDisable(offsetCap);
                }
                current.polygonOffset = polygonOffset;
            }
        }

        void RenderContext::setCullMode(CullMode cullMode)
        {
            auto& current = m_CurrentPipeline.m_RasterizerState.cullMode;
            if (cullMode != current)
            {
                if (cullMode != CullMode::eNone)
                {
                    if (current == CullMode::eNone)
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

        void RenderContext::setDepthClamp(bool enabled)
        {
            auto& current = m_CurrentPipeline.m_RasterizerState.depthClampEnable;
            if (enabled != current)
            {
                enabled ? glEnable(GL_DEPTH_CLAMP) : glDisable(GL_DEPTH_CLAMP);
                current = enabled;
            }
        }

        void RenderContext::setScissorTest(bool enabled)
        {
            auto& current = m_CurrentPipeline.m_RasterizerState.scissorTest;
            if (enabled != current)
            {
                enabled ? glEnable(GL_SCISSOR_TEST) : glDisable(GL_SCISSOR_TEST);
                current = enabled;
            }
        }

        void RenderContext::setBlendState(GLuint index, const BlendState& state)
        {
            auto& current = m_CurrentPipeline.m_BlendStates[index];
            if (state != current)
            {
                if (state.enabled != current.enabled)
                {
                    state.enabled ? glEnablei(GL_BLEND, index) : glDisablei(GL_BLEND, index);
                    current.enabled = state.enabled;
                }
                if (state.enabled)
                {
                    if (state.colorOp != current.colorOp || state.alphaOp != current.alphaOp)
                    {
                        glBlendEquationSeparatei(
                            index, static_cast<GLenum>(state.colorOp), static_cast<GLenum>(state.alphaOp));

                        current.colorOp = state.colorOp;
                        current.alphaOp = state.alphaOp;
                    }
                    if (state.srcColor != current.srcColor || state.destColor != current.destColor ||
                        state.srcAlpha != current.srcAlpha || state.destAlpha != current.destAlpha)
                    {
                        glBlendFuncSeparatei(index,
                                             static_cast<GLenum>(state.srcColor),
                                             static_cast<GLenum>(state.destColor),
                                             static_cast<GLenum>(state.srcAlpha),
                                             static_cast<GLenum>(state.destAlpha));

                        current.srcColor  = state.srcColor;
                        current.destColor = state.destColor;
                        current.srcAlpha  = state.srcAlpha;
                        current.destAlpha = state.destAlpha;
                    }
                }
            }
        }

        namespace framegraph
        {
            void FrameGraphBuffer::create(const Desc& desc, void* allocator)
            {
                handle = static_cast<TransientResources*>(allocator)->acquireBuffer(desc);
            }

            void FrameGraphBuffer::destroy(const Desc& desc, void* allocator) const
            {
                static_cast<TransientResources*>(allocator)->releaseBuffer(desc, handle);
            }

            void FrameGraphTexture::create(const Desc& desc, void* allocator)
            {
                handle = static_cast<TransientResources*>(allocator)->acquireTexture(desc);
            }

            void FrameGraphTexture::destroy(const Desc& desc, void* allocator) const
            {
                static_cast<TransientResources*>(allocator)->releaseTexture(desc, handle);
            }

            void heartbeat(auto& objects, auto& pools, float dt, auto&& deleter)
            {
                constexpr auto kMaxIdleTime = 1.0f; // in seconds

                auto poolIt = pools.begin();
                while (poolIt != pools.end())
                {
                    auto& [_, pool] = *poolIt;
                    if (pool.empty())
                    {
                        poolIt = pools.erase(poolIt);
                    }
                    else
                    {
                        auto objectIt = pool.begin();
                        while (objectIt != pool.cend())
                        {
                            auto& [object, idleTime] = *objectIt;
                            idleTime += dt;
                            if (idleTime >= kMaxIdleTime)
                            {
                                deleter(*object);
                                VGFW_TRACE("[TransientResources] Released resource: {0}", fmt::ptr(object));
                                objectIt = pool.erase(objectIt);
                            }
                            else
                            {
                                ++objectIt;
                            }
                        }
                        ++poolIt;
                    }
                }
                objects.erase(std::remove_if(objects.begin(), objects.end(), [](auto& object) { return !(*object); }),
                              objects.end());
            }

            TransientResources::TransientResources(RenderContext& rc) : m_RenderContext {rc} {}
            TransientResources::~TransientResources()
            {
                for (auto& texture : m_Textures)
                    m_RenderContext.destroy(*texture);
                for (auto& buffer : m_Buffers)
                    m_RenderContext.destroy(*buffer);
            }

            void TransientResources::update(float dt)
            {
                const auto deleter = [&](auto& object) { m_RenderContext.destroy(object); };
                heartbeat(m_Textures, m_TexturePools, dt, deleter);
                heartbeat(m_Buffers, m_BufferPools, dt, deleter);
            }

            Texture* TransientResources::acquireTexture(const FrameGraphTexture::Desc& desc)
            {
                const auto h    = std::hash<FrameGraphTexture::Desc> {}(desc);
                auto&      pool = m_TexturePools[h];
                if (pool.empty())
                {
                    Texture texture;
                    if (desc.depth > 0)
                    {
                        texture = m_RenderContext.createTexture3D(desc.extent, desc.depth, desc.format);
                    }
                    else
                    {
                        texture =
                            m_RenderContext.createTexture2D(desc.extent, desc.format, desc.numMipLevels, desc.layers);
                    }

                    glm::vec4 borderColor {0.0f};
                    auto      addressMode = SamplerAddressMode::eClampToEdge;
                    switch (desc.wrap)
                    {
                        case WrapMode::eClampToEdge:
                            addressMode = SamplerAddressMode::eClampToEdge;
                            break;
                        case WrapMode::eClampToOpaqueBlack:
                            addressMode = SamplerAddressMode::eClampToBorder;
                            borderColor = glm::vec4 {0.0f, 0.0f, 0.0f, 1.0f};
                            break;
                        case WrapMode::eClampToOpaqueWhite:
                            addressMode = SamplerAddressMode::eClampToBorder;
                            borderColor = glm::vec4 {1.0f};
                            break;
                    }
                    SamplerInfo samplerInfo {
                        .minFilter    = desc.filter,
                        .mipmapMode   = desc.numMipLevels > 1 ? MipmapMode::eNearest : MipmapMode::eNone,
                        .magFilter    = desc.filter,
                        .addressModeS = addressMode,
                        .addressModeT = addressMode,
                        .addressModeR = addressMode,
                        .borderColor  = borderColor,
                    };
                    if (desc.shadowSampler)
                        samplerInfo.compareOperator = CompareOp::eLessOrEqual;
                    m_RenderContext.setupSampler(texture, samplerInfo);

                    m_Textures.push_back(std::make_unique<Texture>(std::move(texture)));
                    auto* ptr = m_Textures.back().get();
                    VGFW_TRACE("[TransientResources] Created texture: {0}", fmt::ptr(ptr));
                    return ptr;
                }
                else
                {
                    auto* texture = pool.back().resource;
                    pool.pop_back();
                    return texture;
                }
            }
            void TransientResources::releaseTexture(const FrameGraphTexture::Desc& desc, Texture* texture)
            {
                const auto h = std::hash<FrameGraphTexture::Desc> {}(desc);
                m_TexturePools[h].push_back({texture, 0.0f});
            }

            Buffer* TransientResources::acquireBuffer(const FrameGraphBuffer::Desc& desc)
            {
                const auto h    = std::hash<FrameGraphBuffer::Desc> {}(desc);
                auto&      pool = m_BufferPools[h];
                if (pool.empty())
                {
                    auto buffer = m_RenderContext.createBuffer(desc.size);
                    m_Buffers.push_back(std::make_unique<Buffer>(std::move(buffer)));
                    auto* ptr = m_Buffers.back().get();
                    VGFW_TRACE("[TransientResources] Created buffer: {0}", fmt::ptr(ptr));
                    return ptr;
                }
                else
                {
                    auto* buffer = pool.back().resource;
                    pool.pop_back();
                    return buffer;
                }
            }
            void TransientResources::releaseBuffer(const FrameGraphBuffer::Desc& desc, Buffer* buffer)
            {
                const auto h = std::hash<FrameGraphBuffer::Desc> {}(desc);
                m_BufferPools[h].push_back({std::move(buffer), 0.0f});
            }

            FrameGraphResource importTexture(FrameGraph& fg, const std::string& name, Texture* texture)
            {
                assert(texture && *texture);
                return fg.import <FrameGraphTexture>(name,
                                                     {
                                                         .extent       = texture->getExtent(),
                                                         .numMipLevels = texture->getNumMipLevels(),
                                                         .layers       = texture->getNumLayers(),
                                                         .format       = texture->getPixelFormat(),
                                                     },
                                                     {texture});
            }
            Texture& getTexture(FrameGraphPassResources& resources, FrameGraphResource id)
            {
                return *resources.get<FrameGraphTexture>(id).handle;
            }

            FrameGraphResource importBuffer(FrameGraph& fg, const std::string& name, Buffer* buffer)
            {
                assert(buffer && *buffer);
                return fg.import <FrameGraphBuffer>(name, {.size = buffer->getSize()}, {buffer});
            }
            Buffer& getBuffer(FrameGraphPassResources& resources, FrameGraphResource id)
            {
                return *resources.get<FrameGraphBuffer>(id).handle;
            }
        } // namespace framegraph

        namespace imgui
        {
            void init(bool enableDocking)
            {
                g_EnableDocking = enableDocking;

                // Setup Dear ImGui context
                IMGUI_CHECKVERSION();
                ImGui::CreateContext();
                ImGuiIO& io = ImGui::GetIO();
                (void)io;
                io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
                // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

                if (enableDocking)
                {
                    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable; // Enable Docking
                }

                io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport
                // io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
                // io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

                // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to
                // regular ones.
                ImGuiStyle& style = ImGui::GetStyle();
                if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
                {
                    style.WindowRounding              = 0.0f;
                    style.Colors[ImGuiCol_WindowBg].w = 1.0f;
                    style.PopupRounding = style.TabRounding = 6.0f;
                }

                ImGui_ImplGlfw_InitForOpenGL(
                    static_cast<GLFWwindow*>(getGraphicsContext().getWindow()->getPlatformWindow()), true);
                ImGui_ImplOpenGL3_Init("#version 330");
            }

            void beginFrame()
            {
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                if (g_EnableDocking)
                {
                    static bool               dockSpaceOpen  = true;
                    static ImGuiDockNodeFlags dockSpaceFlags = ImGuiDockNodeFlags_None;

                    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

                    ImGuiViewport* viewport = ImGui::GetMainViewport();
                    ImGui::SetNextWindowPos(viewport->Pos);
                    ImGui::SetNextWindowSize(viewport->Size);
                    ImGui::SetNextWindowViewport(viewport->ID);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
                    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
                    windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                                   ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
                    windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

                    if (dockSpaceFlags & ImGuiDockNodeFlags_PassthruCentralNode)
                    {
                        windowFlags |= ImGuiWindowFlags_NoBackground;
                    }

                    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
                    ImGui::Begin("DockSpaceWindow", &dockSpaceOpen, windowFlags);
                    ImGui::PopStyleVar(3);

                    // DockSpace
                    ImGuiIO&    io          = ImGui::GetIO();
                    ImGuiStyle& style       = ImGui::GetStyle();
                    float       minWinSizeX = style.WindowMinSize.x;
                    float       minWinSizeY = style.WindowMinSize.y;
                    style.WindowMinSize.x   = 350.0f;
                    style.WindowMinSize.y   = 120.0f;
                    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
                    {
                        ImGuiID dockSpaceId = ImGui::GetID("DockSpace");
                        ImGui::DockSpace(dockSpaceId, ImVec2(0.0f, 0.0f), dockSpaceFlags);
                    }
                    style.WindowMinSize.x = minWinSizeX;
                    style.WindowMinSize.y = minWinSizeY;
                }
            }

            void endFrame()
            {
                if (g_EnableDocking)
                {
                    ImGui::End();
                }

                ImGuiIO& io = ImGui::GetIO();
                io.DisplaySize =
                    ImVec2(getGraphicsContext().getWindow()->getWidth(), getGraphicsContext().getWindow()->getHeight());

                ImGui::Render();

                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

                if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
                {
                    GLFWwindow* backupCurrentContext = glfwGetCurrentContext();
                    ImGui::UpdatePlatformWindows();
                    ImGui::RenderPlatformWindowsDefault();
                    glfwMakeContextCurrent(backupCurrentContext);
                }
            }

            void shutdown()
            {
                ImGui_ImplOpenGL3_Shutdown();
                ImGui_ImplGlfw_Shutdown();
                ImGui::DestroyContext();
            }
        } // namespace imgui

        void init(const RendererInitInfo& initInfo)
        {
            g_GraphicsContext.init(initInfo.window);
            g_RenderContext = std::make_shared<RenderContext>();

            imgui::init(initInfo.enableImGuiDocking);

            g_RendererInit = true;
        }

        void beginImGui() { imgui::beginFrame(); }

        void endImGui() { imgui::endFrame(); }

        void present() { g_GraphicsContext.swapBuffers(); }

        void shutdown()
        {
            imgui::shutdown();
            g_GraphicsContext.shutdown();
        }

        bool isLoaded() { return g_RendererInit; }

        GraphicsContext& getGraphicsContext() { return g_GraphicsContext; }
        RenderContext&   getRenderContext() { return *g_RenderContext; }
    } // namespace renderer

    namespace resource
    {
        void MeshPrimitive::build(renderer::VertexFormat::Builder& vertexFormatBuilder, renderer::RenderContext& rc)
        {
            vertexFormat = vertexFormatBuilder.build();
            indexCount   = indices.size();

            bool hasNormal    = !record.normals.empty();
            bool hasTexCoords = !record.texcoords.empty();
            bool hasTangent   = !record.tangents.empty();

            for (uint32_t v = 0; v < vertexCount; ++v)
            {
                // fill position
                vertices.push_back(record.positions[v].x);
                vertices.push_back(record.positions[v].y);
                vertices.push_back(record.positions[v].z);

                // fill normal
                if (hasNormal)
                {
                    vertices.push_back(record.normals[v].x);
                    vertices.push_back(record.normals[v].y);
                    vertices.push_back(record.normals[v].z);
                }

                // fill uv
                if (hasTexCoords)
                {
                    vertices.push_back(record.texcoords[v].x);
                    vertices.push_back(record.texcoords[v].y);
                }

                // fill tangent
                if (hasTangent)
                {
                    vertices.push_back(record.tangents[v].x);
                    vertices.push_back(record.tangents[v].y);
                    vertices.push_back(record.tangents[v].z);
                    vertices.push_back(record.tangents[v].w);
                }
            }

            // Load index buffer & vertex buffer
            auto indexBuf  = rc.createIndexBuffer(renderer::IndexType::eUInt32, indices.size(), indices.data());
            auto vertexBuf = rc.createVertexBuffer(vertexFormat->getStride(), vertexCount, vertices.data());

            indexBuffer  = std::shared_ptr<renderer::IndexBuffer>(new renderer::IndexBuffer {std::move(indexBuf)},
                                                                 renderer::RenderContext::ResourceDeleter {rc});
            vertexBuffer = std::shared_ptr<renderer::VertexBuffer>(new renderer::VertexBuffer {std::move(vertexBuf)},
                                                                   renderer::RenderContext::ResourceDeleter {rc});

            // Load material buffer
            auto materialBuf = rc.createBuffer(sizeof(PrimitiveMaterial), &material);
            materialBuffer   = std::shared_ptr<renderer::Buffer>(new renderer::Buffer {std::move(materialBuf)},
                                                               renderer::RenderContext::ResourceDeleter {rc});
        }

        void MeshPrimitive::draw(renderer::RenderContext& rc) const
        {
            assert(vertexBuffer && indexBuffer);
            rc.draw(*vertexBuffer, *indexBuffer, indexCount, vertexCount);
        }

        void Model::bindMeshPrimitiveTextures(uint32_t                 primitiveIndex,
                                              uint32_t                 unit,
                                              renderer::RenderContext& rc,
                                              std::optional<GLuint>    samplerId) const
        {
            assert(primitiveIndex >= 0 && primitiveIndex < meshPrimitives.size());

            const auto& primitive = meshPrimitives[primitiveIndex];

            for (uint32_t i = 0; i < primitive.textureIndices.size(); ++i)
            {
                rc.bindTexture(unit + i, *textures[primitive.textureIndices[i]], samplerId);
            }
        }
    } // namespace resource

    namespace io
    {
        renderer::Texture* load(const std::filesystem::path& texturePath, renderer::RenderContext& rc, bool flip)
        {
            if (texturePath.empty())
            {
                return nullptr;
            }

            auto       p = std::filesystem::absolute(texturePath);
            const auto h = std::filesystem::hash_value(p);
            if (auto it = g_TextureCache.find(h); it != g_TextureCache.cend())
            {
                auto extent = it->second->getExtent();
                assert(extent.width > 0 && extent.height > 0);

                return it->second;
            }

            stbi_set_flip_vertically_on_load(flip);

            auto* f = stbi__fopen(texturePath.string().c_str(), "rb");
            assert(f);

            const auto hdr = stbi_is_hdr_from_file(f);

            int32_t width, height, numChannels;
            auto*   pixels = hdr ? reinterpret_cast<void*>(stbi_loadf_from_file(f, &width, &height, &numChannels, 0)) :
                                   reinterpret_cast<void*>(stbi_load_from_file(f, &width, &height, &numChannels, 0));
            fclose(f);
            assert(pixels);

            renderer::ImageData imageData {
                .dataType = static_cast<GLenum>(hdr ? GL_FLOAT : GL_UNSIGNED_BYTE),
                .pixels   = pixels,
            };
            renderer::PixelFormat pixelFormat {renderer::PixelFormat::eUnknown};
            switch (numChannels)
            {
                case 1:
                    imageData.format = GL_RED;
                    pixelFormat      = renderer::PixelFormat::eR8_UNorm;
                    break;
                case 3:
                    imageData.format = GL_RGB;
                    pixelFormat      = hdr ? renderer::PixelFormat::eRGB16F : renderer::PixelFormat::eRGB8_UNorm;
                    break;
                case 4:
                    imageData.format = GL_RGBA;
                    pixelFormat      = hdr ? renderer::PixelFormat::eRGBA16F : renderer::PixelFormat::eRGBA8_UNorm;
                    break;

                default:
                    assert(false);
            }

            uint32_t numMipLevels {1u};
            if (math::isPowerOf2(width) && math::isPowerOf2(height))
                numMipLevels = renderer::calcMipLevels(glm::max(width, height));

            auto texture = rc.createTexture2D(
                {static_cast<uint32_t>(width), static_cast<uint32_t>(height)}, pixelFormat, numMipLevels);
            rc.upload(texture, 0, {width, height}, imageData)
                .setupSampler(texture,
                              {
                                  .minFilter     = renderer::TexelFilter::eLinear,
                                  .mipmapMode    = renderer::MipmapMode::eLinear,
                                  .magFilter     = renderer::TexelFilter::eLinear,
                                  .maxAnisotropy = 16.0f,
                              });
            stbi_image_free(pixels);

            if (numMipLevels > 1)
                rc.generateMipmaps(texture);

            auto* newTexture  = new renderer::Texture {std::move(texture)};
            g_TextureCache[h] = newTexture;

            return newTexture;
        }

        void release(const std::filesystem::path& texturePath, renderer::Texture& texture, renderer::RenderContext& rc)
        {
            if (texturePath.empty())
            {
                return;
            }

            auto       p = std::filesystem::absolute(texturePath);
            const auto h = std::filesystem::hash_value(p);
            if (auto it = g_TextureCache.find(h); it != g_TextureCache.cend())
            {
                rc.destroy(texture);
                g_TextureCache.erase(h);
            }
        }

        bool loadOBJ(const std::filesystem::path& modelPath, resource::Model& model, renderer::RenderContext& rc)
        {
            std::string              inputfile = modelPath.generic_string();
            tinyobj::ObjReaderConfig readerConfig;
            readerConfig.mtl_search_path = "./"; // Path to material files

            tinyobj::ObjReader reader;

            if (!reader.ParseFromFile(inputfile, readerConfig))
            {
                if (!reader.Error().empty())
                {
                    VGFW_ERROR("[TinyObjReader] {0}", reader.Error());
                }
                return false;
            }

            if (!reader.Warning().empty())
            {
                VGFW_WARN("[TinyObjReader] {0}", reader.Warning());
            }

            const auto& attrib    = reader.GetAttrib();
            const auto& shapes    = reader.GetShapes();
            const auto& materials = reader.GetMaterials();

            // Loop over shapes
            for (const auto& shape : shapes)
            {
                auto& meshPrimitive = model.meshPrimitives.emplace_back();

                meshPrimitive.name = shape.name;

                auto    vertexFormatBuilder = renderer::VertexFormat::Builder {};
                int32_t attributeOffset     = 0;

                bool hasNormal    = false;
                bool hasTexCoords = false;

                // Loop over faces(polygon)
                size_t indexOffset = 0;
                for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++)
                {
                    size_t fv = static_cast<size_t>(shape.mesh.num_face_vertices[f]);

                    // Loop over vertices in the face.
                    for (size_t v = 0; v < fv; v++)
                    {
                        // access to vertex
                        tinyobj::index_t idx = shape.mesh.indices[indexOffset + v];
                        tinyobj::real_t  vx  = attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 0];
                        tinyobj::real_t  vy  = attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 1];
                        tinyobj::real_t  vz  = attrib.vertices[3 * static_cast<size_t>(idx.vertex_index) + 2];

                        meshPrimitive.record.positions.push_back({vx, vy, vz});

                        // Check if `normal_index` is zero or positive. negative = no normal data
                        if (idx.normal_index >= 0)
                        {
                            hasNormal          = true;
                            tinyobj::real_t nx = attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 0];
                            tinyobj::real_t ny = attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 1];
                            tinyobj::real_t nz = attrib.normals[3 * static_cast<size_t>(idx.normal_index) + 2];

                            meshPrimitive.record.normals.push_back({nx, ny, nz});
                        }

                        // Check if `texcoord_index` is zero or positive. negative = no texcoord data
                        if (idx.texcoord_index >= 0)
                        {
                            hasTexCoords       = true;
                            tinyobj::real_t tx = attrib.texcoords[2 * static_cast<size_t>(idx.texcoord_index) + 0];
                            tinyobj::real_t ty = attrib.texcoords[2 * static_cast<size_t>(idx.texcoord_index) + 1];

                            meshPrimitive.record.texcoords.push_back({tx, ty});
                        }

                        meshPrimitive.indices.push_back(meshPrimitive.indices.size());
                    }
                    indexOffset += fv;
                    meshPrimitive.vertexCount += fv;
                }

                vertexFormatBuilder.setAttribute(
                    renderer::AttributeLocation::ePosition,
                    {.vertType = vgfw::renderer::VertexAttribute::Type::eFloat3, .offset = attributeOffset});
                attributeOffset += sizeof(float) * 3;

                if (hasNormal)
                {
                    vertexFormatBuilder.setAttribute(
                        renderer::AttributeLocation::eNormal_Color,
                        {.vertType = vgfw::renderer::VertexAttribute::Type::eFloat3, .offset = attributeOffset});
                    attributeOffset += sizeof(float) * 3;
                }

                if (hasTexCoords)
                {
                    vertexFormatBuilder.setAttribute(
                        renderer::AttributeLocation::eTexCoords,
                        {.vertType = vgfw::renderer::VertexAttribute::Type::eFloat2, .offset = attributeOffset});
                    attributeOffset += sizeof(float) * 2;
                }

                meshPrimitive.build(vertexFormatBuilder, rc);
            }

            return true;
        }

        bool loadGLTF(const std::filesystem::path& modelPath, resource::Model& model, renderer::RenderContext& rc)
        {
            tinygltf::TinyGLTF loader;
            tinygltf::Model    gltfModel;
            std::string        err;
            std::string        warn;

            bool        ret = false;
            const auto& ext = modelPath.extension();

            if (ext == ".gltf")
            {
                ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, modelPath.generic_string());
            }
            else if (ext == ".glb")
            {
                ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, modelPath.generic_string());
            }
            else
            {
                VGFW_ERROR("[TinyGLTF] Unsupported format");
                return false;
            }

            if (!warn.empty())
            {
                VGFW_WARN("[TinyGLTF] {0}", warn);
            }

            if (!err.empty())
            {
                VGFW_ERROR("[TinyGLTF] {0}", err);
                return false;
            }

            if (!ret)
            {
                VGFW_ERROR("[TinyGLTF] Failed to load GLTF model: {0}", modelPath.generic_string());
                return false;
            }

            // Load textures
            model.textures.resize(gltfModel.textures.size());
            for (const auto& texture : gltfModel.textures)
            {
                const auto&              image         = gltfModel.images[texture.source];
                vgfw::renderer::Texture* loadedTexture = vgfw::io::load(modelPath.parent_path() / image.uri, rc, false);
                model.textures[texture.source]         = loadedTexture;
            }

            // Load materials
            model.materials.resize(gltfModel.materials.size());
            for (const auto& material : gltfModel.materials)
            {
                resource::Material mat {};

                if (material.values.find("baseColorTexture") != material.values.end())
                {
                    int textureIndex          = material.values.at("baseColorTexture").TextureIndex();
                    mat.baseColorTextureIndex = textureIndex;
                }
                if (material.values.find("metallicRoughnessTexture") != material.values.end())
                {
                    int textureIndex                  = material.values.at("metallicRoughnessTexture").TextureIndex();
                    mat.metallicRoughnessTextureIndex = textureIndex;
                }

                mat.normalTextureIndex    = material.normalTexture.index;
                mat.occlusionTextureIndex = material.occlusionTexture.index;
                mat.emissiveTextureIndex  = material.emissiveTexture.index;

                model.materials[&material - &gltfModel.materials[0]] = mat;
            }

            // Load meshes
            for (const auto& mesh : gltfModel.meshes)
            {
                for (const auto& primitive : mesh.primitives)
                {
                    if (primitive.indices < 0)
                        continue;

                    auto& meshPrimitive = model.meshPrimitives.emplace_back();
                    meshPrimitive.name  = mesh.name;

                    const tinygltf::Accessor&   indexAccessor   = gltfModel.accessors[primitive.indices];
                    const tinygltf::BufferView& indexBufferView = gltfModel.bufferViews[indexAccessor.bufferView];
                    const tinygltf::Buffer&     indexBuffer     = gltfModel.buffers[indexBufferView.buffer];

                    const void* indicesData =
                        indexBuffer.data.data() + indexBufferView.byteOffset + indexAccessor.byteOffset;

                    switch (indexAccessor.componentType)
                    {
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                            for (size_t i = 0; i < indexAccessor.count; ++i)
                            {
                                const uint32_t* indices = reinterpret_cast<const uint32_t*>(indicesData);
                                meshPrimitive.indices.push_back(indices[i]);
                            }
                            break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                            for (size_t i = 0; i < indexAccessor.count; ++i)
                            {
                                const uint16_t* indices = reinterpret_cast<const uint16_t*>(indicesData);
                                meshPrimitive.indices.push_back(indices[i]);
                            }
                            break;
                        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                            for (size_t i = 0; i < indexAccessor.count; ++i)
                            {
                                const uint8_t* indices = reinterpret_cast<const uint8_t*>(indicesData);
                                meshPrimitive.indices.push_back(indices[i]);
                            }
                            break;
                    }

                    auto    vertexFormatBuilder = renderer::VertexFormat::Builder {};
                    int32_t attributeOffset     = 0;

                    const tinygltf::Accessor& positionAccessor =
                        gltfModel.accessors[primitive.attributes.find("POSITION")->second];
                    const tinygltf::BufferView& positionBufferView = gltfModel.bufferViews[positionAccessor.bufferView];
                    const tinygltf::Buffer&     positionBuffer     = gltfModel.buffers[positionBufferView.buffer];

                    for (size_t i = 0; i < positionAccessor.count; ++i)
                    {
                        const float* positions = reinterpret_cast<const float*>(
                            positionBuffer.data.data() + positionBufferView.byteOffset + positionAccessor.byteOffset);
                        meshPrimitive.record.positions.push_back(
                            {positions[3 * i + 0], positions[3 * i + 1], positions[3 * i + 2]});
                    }

                    vertexFormatBuilder.setAttribute(
                        renderer::AttributeLocation::ePosition,
                        {.vertType = vgfw::renderer::VertexAttribute::Type::eFloat3, .offset = attributeOffset});
                    attributeOffset += sizeof(float) * 3;

                    if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
                    {
                        const tinygltf::Accessor& normalAccessor =
                            gltfModel.accessors[primitive.attributes.find("NORMAL")->second];
                        const tinygltf::BufferView& normalBufferView = gltfModel.bufferViews[normalAccessor.bufferView];
                        const tinygltf::Buffer&     normalBuffer     = gltfModel.buffers[normalBufferView.buffer];

                        for (size_t i = 0; i < normalAccessor.count; ++i)
                        {
                            const float* normals = reinterpret_cast<const float*>(
                                normalBuffer.data.data() + normalBufferView.byteOffset + normalAccessor.byteOffset);
                            meshPrimitive.record.normals.push_back(
                                {normals[3 * i + 0], normals[3 * i + 1], normals[3 * i + 2]});
                        }

                        vertexFormatBuilder.setAttribute(
                            renderer::AttributeLocation::eNormal_Color,
                            {.vertType = vgfw::renderer::VertexAttribute::Type::eFloat3, .offset = attributeOffset});
                        attributeOffset += sizeof(float) * 3;
                    }

                    if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
                    {
                        const tinygltf::Accessor& texCoordAccessor =
                            gltfModel.accessors[primitive.attributes.find("TEXCOORD_0")->second];
                        const tinygltf::BufferView& texCoordBufferView =
                            gltfModel.bufferViews[texCoordAccessor.bufferView];
                        const tinygltf::Buffer& texCoordBuffer = gltfModel.buffers[texCoordBufferView.buffer];

                        for (size_t i = 0; i < texCoordAccessor.count; ++i)
                        {
                            const float* texCoords = reinterpret_cast<const float*>(texCoordBuffer.data.data() +
                                                                                    texCoordBufferView.byteOffset +
                                                                                    texCoordAccessor.byteOffset);

                            meshPrimitive.record.texcoords.push_back({texCoords[2 * i + 0], texCoords[2 * i + 1]});
                        }

                        vertexFormatBuilder.setAttribute(
                            renderer::AttributeLocation::eTexCoords,
                            {.vertType = vgfw::renderer::VertexAttribute::Type::eFloat2, .offset = attributeOffset});
                        attributeOffset += sizeof(float) * 2;
                    }

                    if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
                    {
                        const tinygltf::Accessor& tangentAccessor =
                            gltfModel.accessors[primitive.attributes.find("TANGENT")->second];
                        const tinygltf::BufferView& tangentBufferView =
                            gltfModel.bufferViews[tangentAccessor.bufferView];
                        const tinygltf::Buffer& tangentBuffer = gltfModel.buffers[tangentBufferView.buffer];

                        for (size_t i = 0; i < tangentAccessor.count; ++i)
                        {
                            const float* tangents = reinterpret_cast<const float*>(
                                tangentBuffer.data.data() + tangentBufferView.byteOffset + tangentAccessor.byteOffset);
                            meshPrimitive.record.tangents.push_back(
                                {tangents[4 * i + 0], tangents[4 * i + 1], tangents[4 * i + 2], tangents[4 * i + 3]});
                        }

                        vertexFormatBuilder.setAttribute(
                            renderer::AttributeLocation::eTangent,
                            {.vertType = vgfw::renderer::VertexAttribute::Type::eFloat4, .offset = attributeOffset});
                        attributeOffset += sizeof(float) * 4;
                    }

                    // TODO: Additional attributes such as joint indices and weights can be added here

                    meshPrimitive.materialIndex = primitive.material;
                    meshPrimitive.vertexCount   = positionAccessor.count;

                    const auto& material = model.materials[meshPrimitive.materialIndex];

                    uint32_t textureIndex = 0;

                    if (material.baseColorTextureIndex != -1)
                    {
                        meshPrimitive.material.baseColorTextureIndex = textureIndex++;
                        meshPrimitive.textureIndices.push_back(material.baseColorTextureIndex);
                    }

                    if (material.metallicRoughnessTextureIndex != -1)
                    {
                        meshPrimitive.material.metallicRoughnessTextureIndex = textureIndex++;
                        meshPrimitive.textureIndices.push_back(material.metallicRoughnessTextureIndex);
                    }

                    if (material.normalTextureIndex != -1)
                    {
                        meshPrimitive.material.normalTextureIndex = textureIndex++;
                        meshPrimitive.textureIndices.push_back(material.normalTextureIndex);
                    }

                    if (material.occlusionTextureIndex != -1)
                    {
                        meshPrimitive.material.occlusionTextureIndex = textureIndex++;
                        meshPrimitive.textureIndices.push_back(material.occlusionTextureIndex);
                    }

                    if (material.emissiveTextureIndex != -1)
                    {
                        meshPrimitive.material.emissiveTextureIndex = textureIndex++;
                        meshPrimitive.textureIndices.push_back(material.emissiveTextureIndex);
                    }

                    meshPrimitive.build(vertexFormatBuilder, rc);
                }
            }

            return true;
        }

        bool load(const std::filesystem::path& modelPath, resource::Model& model, renderer::RenderContext& rc)
        {
            const auto& ext = modelPath.extension();
            if (ext == ".obj")
            {
                return loadOBJ(modelPath, model, rc);
            }
            else if (ext == ".gltf" || ext == ".glb")
            {
                return loadGLTF(modelPath, model, rc);
            }

            return false;
        }
    } // namespace io

    bool init()
    {
        log::init();

        return true;
    }

    void shutdown()
    {
        if (renderer::isLoaded())
        {
            renderer::shutdown();
        }

        log::shutdown();
    }
} // namespace vgfw