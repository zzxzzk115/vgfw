#include "vgfw.hpp"

#include <chrono>

const char* vertexShaderSource = R"(
#version 450

layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aTexCoords;

layout(location = 0) out vec2 vTexCoords;

layout(location = 0) uniform mat4 model;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vTexCoords = aTexCoords;
}
)";

const char* fragmentShaderSource = R"(
#version 450

layout(location = 0) in vec2 vTexCoords;

layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform sampler2D cubeTexture;

void main()
{
    FragColor = vec4(texture(cubeTexture, vTexCoords).rgb, 1.0);
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
    auto window = vgfw::window::create({.Title = "02-cube", .EnableMSAA = true, .AASample = 8});

    // Init renderer
    vgfw::renderer::init({.Window = window});

    // Get graphics & render context
    auto& rc = vgfw::renderer::getRenderContext();

    // Build vertex format
    auto vertexFormat = vgfw::renderer::VertexFormat::Builder {}
                            .SetAttribute(vgfw::renderer::AttributeLocation::Position,
                                          {.VertType = vgfw::renderer::VertexAttribute::Type::Float3, .Offset = 0})
                            .SetAttribute(vgfw::renderer::AttributeLocation::TexCoords,
                                          {.VertType = vgfw::renderer::VertexAttribute::Type::Float2, .Offset = 12})
                            .Build();

    // Get vertex array object
    auto vao = rc.GetVertexArray(vertexFormat->GetAttributes());

    // Create shader program
    auto program = rc.CreateGraphicsProgram(vertexShaderSource, fragmentShaderSource);

    // Build a graphics pipeline
    auto graphicsPipeline = vgfw::renderer::GraphicsPipeline::Builder {}
                                .SetDepthStencil({
                                    .DepthTest      = true,
                                    .DepthWrite     = true,
                                    .DepthCompareOp = vgfw::renderer::CompareOp::Less,
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
    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };
    // clang-format on

    // Create index buffer & vertex buffer
    auto vertexBuffer = rc.CreateVertexBuffer(vertexFormat->GetStride(), 36, vertices);

    // Load texture
    auto* texture = vgfw::io::load("assets/textures/awesomeface.png", rc);

    // Start time
    auto startTime = std::chrono::high_resolution_clock::now();

    float fov = 60.0f;

    // Main loop
    while (!window->ShouldClose())
    {
        window->OnTick();

        // Calculate the elapsed time
        auto  currentTime = std::chrono::high_resolution_clock::now();
        float time        = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        // Create the model matrix
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), time, glm::vec3(0.5f, 1.0f, 0.0f));

        // Create the view matrix
        glm::mat4 view = glm::lookAt(glm::vec3(1.5f, 1.5f, 1.5f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        // Create the projection matrix
        glm::mat4 projection =
            glm::perspective(glm::radians(fov), window->GetWidth() * 1.0f / window->GetHeight(), 0.1f, 100.0f);

        // Render
        rc.BeginRendering({.Extent = {.Width = window->GetWidth(), .Height = window->GetHeight()}}, glm::vec4 {0.2f, 0.3f, 0.3f, 1.0f}, 1.0f);
        rc.BindGraphicsPipeline(graphicsPipeline)
            .SetUniformMat4("model", model)
            .SetUniformMat4("view", view)
            .SetUniformMat4("projection", projection)
            .BindTexture(0, *texture)
            .Draw(vertexBuffer, {}, 0, 36);

        vgfw::renderer::beginImGui();
        ImGui::Begin("Cube");
        ImGui::SliderFloat("FOV", &fov, 1.0f, 179.0f);
        ImGui::End();
        vgfw::renderer::endImGui();

        vgfw::renderer::present();
    }

    // Cleanup
    rc.Destroy(vertexBuffer);
    vgfw::shutdown();

    return 0;
}