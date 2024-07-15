#define VGFW_IMPLEMENTATION
#include "vgfw.hpp"

const char* vertexShaderSource = R"(
#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

out vec3 vertexColor;

void main()
{
    gl_Position = vec4(aPos, 1.0);
    vertexColor = aColor;
}
)";

const char* fragmentShaderSource = R"(
#version 450

in vec3 vertexColor;

out vec4 FragColor;

void main()
{
    FragColor = vec4(vertexColor, 1.0);
}
)";

int main()
{
    // Init VGFW
    if (!vgfw::init())
    {
        std::cerr << "Failed to initialize VGFW" << std::endl;
        return -1;
    }

    // Create a window instance
    auto window = vgfw::window::create({.title = "01-hello-triangle", .aaSample = vgfw::window::AASample::e8});

    // Init renderer
    vgfw::renderer::init({.window = window});

    // Get render context
    auto& rc = vgfw::renderer::getRenderContext();

    // Build vertex format
    auto vertexFormat = vgfw::renderer::VertexFormat::Builder {}
                            .setAttribute(vgfw::renderer::AttributeLocation::ePosition,
                                          {.vertType = vgfw::renderer::VertexAttribute::Type::eFloat3, .offset = 0})
                            .setAttribute(vgfw::renderer::AttributeLocation::eNormal_Color,
                                          {.vertType = vgfw::renderer::VertexAttribute::Type::eFloat3, .offset = 12})
                            .build();

    // Get vertex array object
    auto vao = rc.getVertexArray(vertexFormat->getAttributes());

    // Create shader program
    auto program = rc.createGraphicsProgram(vertexShaderSource, fragmentShaderSource);

    // Build a graphics pipeline
    auto graphicsPipeline = vgfw::renderer::GraphicsPipeline::Builder {}
                                .setDepthStencil({
                                    .depthTest      = false,
                                    .depthWrite     = true,
                                    .depthCompareOp = vgfw::renderer::CompareOp::eLess,
                                })
                                .setRasterizerState({
                                    .polygonMode = vgfw::renderer::PolygonMode::eFill,
                                    .cullMode    = vgfw::renderer::CullMode::eBack,
                                    .scissorTest = false,
                                })
                                .setVAO(vao)
                                .setShaderProgram(program)
                                .build();

    // clang-format off
    // Vertices
    float vertices[] = {
        // Position                                    // Color
         0.0f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,
         0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f
    };

    // Indices
    uint32_t indices[] = {
        0, 1, 2
    };
    // clang-format on

    // Create index buffer & vertex buffer
    auto indexBuffer  = rc.createIndexBuffer(vgfw::renderer::IndexType::eUInt32, 3, indices);
    auto vertexBuffer = rc.createVertexBuffer(vertexFormat->getStride(), 3, vertices);

    // Main loop
    while (!window->shouldClose())
    {
        window->onTick();

        vgfw::renderer::beginFrame();

        // Render
        rc.beginRendering({.extent = {.width = window->getWidth(), .height = window->getHeight()}},
                          glm::vec4 {0.2f, 0.3f, 0.3f, 1.0f});
        rc.bindGraphicsPipeline(graphicsPipeline).draw(vertexBuffer, indexBuffer, 3, 3);

        ImGui::Begin("Triangle");
        ImGui::Text("Hello, VGFW Triangle!");
        ImGui::End();

        vgfw::renderer::endFrame();

        vgfw::renderer::present();
    }

    // Cleanup
    rc.destroy(indexBuffer);
    rc.destroy(vertexBuffer);
    vgfw::shutdown();

    return 0;
}