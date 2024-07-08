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
    auto window = vgfw::window::create({.title = "02-cube", .enableMSAA = true, .aaSample = 8});

    // Init renderer
    vgfw::renderer::init({.window = window});

    // Get graphics & render context
    auto& rc = vgfw::renderer::getRenderContext();

    // Build vertex format
    auto vertexFormat = vgfw::renderer::VertexFormat::Builder {}
                            .setAttribute(vgfw::renderer::AttributeLocation::Position,
                                          {.vertType = vgfw::renderer::VertexAttribute::Type::Float3, .offset = 0})
                            .setAttribute(vgfw::renderer::AttributeLocation::TexCoords,
                                          {.vertType = vgfw::renderer::VertexAttribute::Type::Float2, .offset = 12})
                            .build();

    // Get vertex array object
    auto vao = rc.getVertexArray(vertexFormat->getAttributes());

    // Create shader program
    auto program = rc.createGraphicsProgram(vertexShaderSource, fragmentShaderSource);

    // Build a graphics pipeline
    auto graphicsPipeline = vgfw::renderer::GraphicsPipeline::Builder {}
                                .setDepthStencil({
                                    .depthTest      = true,
                                    .depthWrite     = true,
                                    .depthCompareOp = vgfw::renderer::CompareOp::Less,
                                })
                                .setRasterizerState({
                                    .polygonMode = vgfw::renderer::PolygonMode::Fill,
                                    .cullMode    = vgfw::renderer::CullMode::Back,
                                    .scissorTest = false,
                                })
                                .setVAO(vao)
                                .setShaderProgram(program)
                                .build();

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
    auto vertexBuffer = rc.createVertexBuffer(vertexFormat->getStride(), 36, vertices);

    // Load texture
    auto* texture = vgfw::io::load("assets/textures/awesomeface.png", rc);

    // Start time
    auto startTime = std::chrono::high_resolution_clock::now();

    float fov = 60.0f;

    // Main loop
    while (!window->shouldClose())
    {
        window->onTick();

        // Calculate the elapsed time
        auto  currentTime = std::chrono::high_resolution_clock::now();
        float time        = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        // Create the model matrix
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), time, glm::vec3(0.5f, 1.0f, 0.0f));

        // Create the view matrix
        glm::mat4 view = glm::lookAt(glm::vec3(1.5f, 1.5f, 1.5f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        // Create the projection matrix
        glm::mat4 projection =
            glm::perspective(glm::radians(fov), window->getWidth() * 1.0f / window->getHeight(), 0.1f, 100.0f);

        // Render
        rc.beginRendering({.extent = {.width = window->getWidth(), .height = window->getHeight()}},
                          glm::vec4 {0.2f, 0.3f, 0.3f, 1.0f},
                          1.0f);
        rc.bindGraphicsPipeline(graphicsPipeline)
            .setUniformMat4("model", model)
            .setUniformMat4("view", view)
            .setUniformMat4("projection", projection)
            .bindTexture(0, *texture)
            .draw(vertexBuffer, {}, 0, 36);

        vgfw::renderer::beginImGui();
        ImGui::Begin("Cube");
        ImGui::SliderFloat("Camera FOV", &fov, 1.0f, 179.0f);
        ImGui::End();
        vgfw::renderer::endImGui();

        vgfw::renderer::present();
    }

    // Cleanup
    rc.destroy(vertexBuffer);
    vgfw::shutdown();

    return 0;
}