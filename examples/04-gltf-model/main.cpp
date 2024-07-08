#include "vgfw.hpp"

#include <chrono>

const char* vertexShaderSource = R"(
#version 450

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

layout(location = 0) out vec2 vTexCoords;
layout(location = 1) out vec3 vFragPos;
layout(location = 2) out vec3 vNormal;

layout(location = 0) uniform mat4 model;
layout(location = 1) uniform mat4 view;
layout(location = 2) uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    vTexCoords = aTexCoords;
    vFragPos = vec3(model * vec4(aPos, 1.0));
    vNormal = mat3(transpose(inverse(model))) * aNormal;
}
)";

const char* fragmentShaderSource = R"(
#version 450

layout(location = 0) in vec2 vTexCoords;
layout(location = 1) in vec3 vFragPos;
layout(location = 2) in vec3 vNormal;

layout(location = 0) out vec4 FragColor;

layout(location = 3) uniform vec3 lightPos;
layout(location = 4) uniform vec3 viewPos;
layout(location = 5) uniform vec3 lightColor;
layout(location = 6) uniform vec3 objectColor;

void main()
{
    // Ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor;

    // Diffuse
    vec3 norm = normalize(vNormal);
    vec3 lightDir = normalize(lightPos - vFragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - vFragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * objectColor;
    FragColor = vec4(result, 1.0);
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
    auto window = vgfw::window::create({.Title = "04-gltf-model", .EnableMSAA = true, .AASample = 8});

    // Init renderer
    vgfw::renderer::init({.Window = window});

    // Get graphics & render context
    auto& rc = vgfw::renderer::getRenderContext();

    // Build vertex format
    auto vertexFormat = vgfw::renderer::VertexFormat::Builder {}.BuildDefault();

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

    // Load model
    vgfw::resource::Model suzanneModel {};
    if (!vgfw::io::load("assets/models/Suzanne.gltf", suzanneModel))
    {
        return -1;
    }

    // Create index buffer & vertex buffer
    auto indexBuffer  = rc.CreateIndexBuffer(vgfw::renderer::IndexType::UInt32,
                                            suzanneModel.Meshes[0].Indices.size(),
                                            suzanneModel.Meshes[0].Indices.data());
    auto vertexBuffer = rc.CreateVertexBuffer(
        vertexFormat->GetStride(), suzanneModel.Meshes[0].Vertices.size(), suzanneModel.Meshes[0].Vertices.data());

    // Start time
    auto startTime = std::chrono::high_resolution_clock::now();

    // Camera properties
    float     fov = 60.0f;
    glm::vec3 viewPos(0.0f, 0.0f, 3.0f);

    // Light properties
    glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    glm::vec3 objectColor(1.0f, 1.0f, 1.0f);

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
        glm::mat4 view = glm::lookAt(viewPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        // Create the projection matrix
        glm::mat4 projection =
            glm::perspective(glm::radians(fov), window->GetWidth() * 1.0f / window->GetHeight(), 0.1f, 100.0f);

        // Render
        rc.BeginRendering({.Extent = {.Width = window->GetWidth(), .Height = window->GetHeight()}},
                          glm::vec4 {0.2f, 0.3f, 0.3f, 1.0f},
                          1.0f);
        rc.BindGraphicsPipeline(graphicsPipeline)
            .SetUniformMat4("model", model)
            .SetUniformMat4("view", view)
            .SetUniformMat4("projection", projection)
            .SetUniformVec3("lightPos", lightPos)
            .SetUniformVec3("viewPos", viewPos)
            .SetUniformVec3("lightColor", lightColor)
            .SetUniformVec3("objectColor", objectColor)
            .Draw(vertexBuffer,
                  indexBuffer,
                  suzanneModel.Meshes[0].Indices.size(),
                  suzanneModel.Meshes[0].Vertices.size());

        vgfw::renderer::beginImGui();
        ImGui::Begin("GLTF Model");
        ImGui::SliderFloat("Camera FOV", &fov, 1.0f, 179.0f);
        ImGui::DragFloat3("Camera Position", glm::value_ptr(viewPos));
        ImGui::DragFloat3("Light Position", glm::value_ptr(lightPos));
        ImGui::ColorEdit3("Light Color", glm::value_ptr(lightColor));
        ImGui::ColorEdit3("Object Color", glm::value_ptr(objectColor));
        ImGui::End();
        vgfw::renderer::endImGui();

        vgfw::renderer::present();
    }

    // Cleanup
    rc.Destroy(indexBuffer);
    rc.Destroy(vertexBuffer);
    vgfw::shutdown();

    return 0;
}