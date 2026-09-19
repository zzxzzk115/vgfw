// Reject the preferred context in this test only; GLFW must create a real 3.3 context.
#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <string>
#include <stdexcept>
#include <iostream>

static std::string testMode;
static bool rejected = false;
static int pendingError = GLFW_NO_ERROR;
static GLFWwindow* createTestWindow(int w, int h, const char* title, GLFWmonitor* monitor, GLFWwindow* share)
{
    if (testMode == "fallback" && !rejected)
    {
        rejected = true;
        pendingError = GLFW_VERSION_UNAVAILABLE;
        return nullptr;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    return glfwCreateWindow(w, h, title, monitor, share);
}
static int getTestError(const char** description)
{
    if (pendingError)
    {
        const int error = pendingError;
        pendingError = GLFW_NO_ERROR;
        if (description) *description = "Test: preferred context unavailable";
        return error;
    }
    return glfwGetError(description);
}
static int loadTestGL(GLADloadproc proc)
{
    if (testMode == "loader-failure") return 0;
    const int result = gladLoadGLLoader(proc);
    if (testMode == "missing-function") glad_glGenVertexArrays = nullptr;
    return result;
}
#define glfwCreateWindow createTestWindow
#define glfwGetError getTestError
#define gladLoadGLLoader loadTestGL
#define VGFW_IMPLEMENTATION
#define VGFW_ENABLE_GL_DEBUG
#include "vgfw.hpp"
#undef glfwCreateWindow
#undef glfwGetError
#undef gladLoadGLLoader

static void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

int main(int argc, char** argv)
try
{
    if (argc > 1) testMode = argv[1];
    vgfw::init();
    auto window = vgfw::window::create({.title = "VGFW smoke", .width = 64, .height = 64});
    try { vgfw::renderer::init({.window = window}); }
    catch (const std::runtime_error& error)
    {
        const std::string message = error.what();
        if ((testMode == "loader-failure" && message.find("Failed to load OpenGL") != std::string::npos) ||
            (testMode == "missing-function" && message.find("functions are unavailable") != std::string::npos))
        {
            glfwDestroyWindow(static_cast<GLFWwindow*>(window->getPlatformWindow()));
            glfwTerminate();
            vgfw::log::shutdown();
            std::cout << "Expected initialization failure: " << message << '\n';
            return 0;
        }
        throw;
    }
    require(testMode != "loader-failure" && testMode != "missing-function", "Missing initialization error");
    const bool legacy = testMode == "fallback";
    if (legacy)
    {
        require(rejected && GLVersion.major == 3 && GLVersion.minor == 3, "Test requires a real OpenGL 3.3 context");
        require(!vgfw::renderer::getGraphicsContext().isSupportDSA(), "DSA must be disabled on 3.3");
    }
    using namespace vgfw::renderer;
    auto& rc = vgfw::renderer::getRenderContext();
    auto buffer = rc.createBuffer(32);
    const uint32_t value = 123;
    rc.upload(buffer, 4, sizeof(value), &value);
    uint32_t result = 0;
    rc.bindUniformBuffer(0, buffer);
    glGetBufferSubData(GL_UNIFORM_BUFFER, 4, sizeof(result), &result);
    require(result == value, "Buffer upload failed");
    auto* mapped = static_cast<uint32_t*>(rc.map(buffer));
    require(mapped != nullptr, "Buffer mapping failed");
    mapped[0] = 77;
    rc.unmap(buffer);
    rc.clear(buffer);
    glGetBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(result), &result);
    require(result == 0, "Buffer clear failed");

    auto texture = rc.createTexture2D({2, 2}, PixelFormat::eRGBA8_UNorm, 2);
    const uint32_t red[4] = {0xff0000ff, 0xff0000ff, 0xff0000ff, 0xff0000ff};
    rc.upload(texture, 0, glm::uvec2(2), {GL_RGBA, GL_UNSIGNED_BYTE, red});
    rc.setupSampler(texture, {.minFilter = TexelFilter::eNearest, .magFilter = TexelFilter::eNearest});
    rc.generateMipmaps(texture);
    auto sampler = rc.createSampler({.minFilter = TexelFilter::eNearest, .magFilter = TexelFilter::eNearest});

    auto output = rc.createTexture2D({16, 16}, PixelFormat::eRGBA8_UNorm);
    auto depth = rc.createTexture2D({16, 16}, PixelFormat::eDepth24);
    auto format = VertexFormat::Builder{}.setAttribute(AttributeLocation::ePosition,
        {.vertType = VertexAttribute::Type::eFloat2, .offset = 0}).build();
    const float vertices[] = {-1, -1, 3, -1, -1, 3};
    const uint32_t indices[] = {0, 1, 2};
    auto vb = rc.createVertexBuffer(2 * sizeof(float), 3, vertices);
    auto ib = rc.createIndexBuffer(IndexType::eUInt32, 3, indices);
    const auto program = rc.createGraphicsProgram(R"(
#version 330 core
layout(location=0) in vec2 position;
uniform mat4 transform;
void main() { gl_Position = transform * vec4(position, 0.0, 1.0); }
)", R"(
#version 330 core
uniform sampler2D image;
uniform vec4 tint;
out vec4 color;
void main() { color = texture(image, vec2(0.5)) * tint; }
)");
    auto pipeline = GraphicsPipeline::Builder{}.setShaderProgram(program)
        .setVAO(rc.getVertexArray(format->getAttributes())).build();
    const auto fbo = rc.beginRendering({.area = {.extent = {16,16}},
        .colorAttachments = {{output, 0, {}, {}, glm::vec4(0)}},
        .depthAttachment = AttachmentInfo{depth, 0, {}, {}, 1.0f}});
    rc.bindGraphicsPipeline(pipeline).bindTexture(0, texture, sampler)
        .setUniform1i("image", 0).setUniformVec4("tint", glm::vec4(1)).setUniformMat4("transform", glm::mat4(1));
    { DebugMarker marker("smoke"); rc.draw(vb, ib, {.numVertices=3, .numIndices=3}); }
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
    unsigned char pixel[4] = {};
    glReadPixels(8, 8, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixel);
    require(pixel[0] > 240 && pixel[1] < 10 && pixel[2] < 10, "Textured indexed draw failed");
    rc.endRendering(fbo);

    // Exercise arrays, volumes, cubemap faces, clears, and depth-only framebuffers.
    auto array = rc.createTexture2D({2,2}, PixelFormat::eRGBA8_UNorm, 1, 2);
    auto volume = rc.createTexture3D({2,2}, 2, PixelFormat::eRGBA8_UNorm);
    auto cube = rc.createCubemap(2, PixelFormat::eRGBA8_UNorm);
    rc.upload(array, 0, {2,2,0}, 0, 1, {GL_RGBA, GL_UNSIGNED_BYTE, red});
    const uint32_t voxels[8] = {1,2,3,4,5,6,7,8};
    rc.upload(volume, 0, {2,2,2}, 0, 0, {GL_RGBA, GL_UNSIGNED_BYTE, voxels});
    rc.upload(cube, 0, 2, glm::uvec2(2), {GL_RGBA, GL_UNSIGNED_BYTE, red});
    for (auto* t : {&texture, &array, &volume, &cube})
    {
        rc.clear(*t);
        glActiveTexture(GL_TEXTURE0);
        rc.bindTexture(0, *t);
        uint32_t pixels[8];
        std::fill(std::begin(pixels), std::end(pixels), 0xffffffff);
        glGetTexImage(t == &cube ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + 2 : t->getType(), 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        require(pixels[0] == 0 && pixels[3] == 0, "Texture clear failed");
    }
    const auto cubeFbo = rc.beginRendering({.area = {.extent={2,2}},
        .colorAttachments={{cube, 0, {}, 3, glm::vec4(0,1,0,1)}}});
    require(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Cubemap framebuffer failed");
    rc.endRendering(cubeFbo);
    const auto depthFbo = rc.beginRendering({.area={.extent={16,16}}, .depthAttachment=AttachmentInfo{depth,0,{},{},1.0f}});
    require(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Depth framebuffer failed");
    rc.endRendering(depthFbo);
    if (legacy)
    {
        bool denied = false;
        try { rc.createComputeProgram("#version 430\nvoid main(){}"); }
        catch (const std::runtime_error&) { denied = true; }
        require(denied, "Compute must be rejected on 3.3");
    }
    require(glGetError() == GL_NO_ERROR, "Unexpected OpenGL error");
    for (auto* t : {&texture, &output, &depth, &array, &volume, &cube}) rc.destroy(*t);
    rc.destroy(buffer); rc.destroy(vb); rc.destroy(ib); rc.destroy(pipeline);
    glDeleteSamplers(1, &sampler);
    vgfw::shutdown();
    std::cout << "Render smoke passed: " << (legacy ? "3.3 fallback" : "preferred context") << '\n';
    return 0;
}
catch (const std::exception& error)
{
    std::cerr << error.what() << '\n';
    return 1;
}
