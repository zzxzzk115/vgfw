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
    auto window = vgfw::window::create(
        {.Width = 800, .Height = 600, .Title = "01-hello-triangle", .EnableMSAA = true, .AASample = 8});

    // Init renderer
    vgfw::renderer::init(window);

    // Get graphics & render context
    auto& rc = vgfw::renderer::getRenderContext();

    // Build vertex format
    auto vertexFormat = vgfw::renderer::VertexFormat::Builder {}
                            .SetAttribute(vgfw::renderer::AttributeLocation::Position,
                                          {.VertType = vgfw::renderer::VertexAttribute::Type::Float3, .Offset = 0})
                            .SetAttribute(vgfw::renderer::AttributeLocation::Normal_Color,
                                          {.VertType = vgfw::renderer::VertexAttribute::Type::Float3, .Offset = 12})
                            .Build();

    // Get vertex array object
    auto vao = rc.GetVertexArray(vertexFormat->GetAttributes());

    // Create shader program
    auto program = rc.CreateGraphicsProgram(vertexShaderSource, fragmentShaderSource);

    // Build a graphics pipeline
    auto graphicsPipeline = vgfw::renderer::GraphicsPipeline::Builder {}
                                .SetDepthStencil({
                                    .DepthTest      = false,
                                    .DepthWrite     = false,
                                    .DepthCompareOp = vgfw::renderer::CompareOp::LessOrEqual,
                                })
                                .SetRasterizerState({
                                    .PolygonMode = vgfw::renderer::PolygonMode::Fill,
                                    .CullMode    = vgfw::renderer::CullMode::Back,
                                    .ScissorTest = false,
                                })
                                .SetVAO(vao)
                                .SetShaderProgram(program)
                                .Build();

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
    auto indexBuffer  = rc.CreateIndexBuffer(vgfw::renderer::IndexType::UInt32, 3, indices);
    auto vertexBuffer = rc.CreateVertexBuffer(vertexFormat->GetStride(), 3, vertices);

    // Main loop
    while (!window->ShouldClose())
    {
        window->OnTick();

        // Render
        rc.BeginRendering({.Extent = {.Width = 800, .Height = 600}}, glm::vec4 {0.2f, 0.3f, 0.3f, 1.0f});
        rc.BindGraphicsPipeline(graphicsPipeline).Draw(vertexBuffer, indexBuffer, 3, 3);

        vgfw::renderer::present();
    }

    // Cleanup
    rc.Destroy(indexBuffer);
    rc.Destroy(vertexBuffer);
    vgfw::shutdown();

    return 0;
}