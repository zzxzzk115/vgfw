#include "vgfw.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

glm::vec3 getFrontVector(float yaw, float pitch);

int main()
{
    // Init VGFW
    if (!vgfw::init())
    {
        std::cerr << "Failed to initialize VGFW" << std::endl;
        return -1;
    }

    // Create a window instance
    auto window = vgfw::window::create({.Title = "05-deferred", .EnableMSAA = true, .AASample = 8});

    // Init renderer
    vgfw::renderer::init({.Window = window});

    // Get graphics & render context
    auto& rc = vgfw::renderer::getRenderContext();

    // Build vertex format
    auto vertexFormat = vgfw::renderer::VertexFormat::Builder {}.BuildDefault();

    // Get vertex array object
    auto vao = rc.GetVertexArray(vertexFormat->GetAttributes());

    // Create shader program
    auto program = rc.CreateGraphicsProgram(vgfw::utils::readFileAllText("shaders/default.vert"),
                                            vgfw::utils::readFileAllText("shaders/default.frag"));

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
    vgfw::resource::Model sponza {};
    if (!vgfw::io::load("assets/models/Sponza/glTF/Sponza.gltf", sponza))
    {
        return -1;
    }

    // Camera properties
    glm::vec3 cameraPos(-1150, 200, -45);
    float     fov   = 60.0f;
    float     yaw   = 90.0f;
    float     pitch = 0.0f;

    // Main loop
    while (!window->ShouldClose())
    {
        window->OnTick();

        // Create the view matrix
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + getFrontVector(yaw, pitch), glm::vec3(0.0f, 1.0f, 0.0f));

        // Create the projection matrix
        glm::mat4 projection =
            glm::perspective(glm::radians(fov), window->GetWidth() * 1.0f / window->GetHeight(), 1.0f, 10000.0f);

        // Render
        rc.BeginRendering({.Extent = {.Width = window->GetWidth(), .Height = window->GetHeight()}},
                          glm::vec4 {0.2f, 0.3f, 0.3f, 1.0f},
                          1.0f);

        for (const auto& meshPrimitive : sponza.Meshes)
        {
            rc.BindGraphicsPipeline(graphicsPipeline)
                .SetUniformMat4("view", view)
                .SetUniformMat4("projection", projection)
                .SetUniformVec3("viewPos", cameraPos)
                .BindTexture(0,
                             *sponza.TextureMap[sponza.MaterialMap[meshPrimitive.MaterialIndex].BaseColorTextureIndex])
                .Draw(*meshPrimitive.VertexBuf,
                      *meshPrimitive.IndexBuf,
                      meshPrimitive.Indices.size(),
                      meshPrimitive.Vertices.size());
        }

        vgfw::renderer::beginImGui();
        ImGui::Begin("Deferred Rendering");
        ImGui::SliderFloat("Camera FOV", &fov, 1.0f, 179.0f);
        ImGui::DragFloat3("Camera Position", glm::value_ptr(cameraPos));
        ImGui::End();
        vgfw::renderer::endImGui();

        vgfw::renderer::present();
    }

    // Cleanup
    vgfw::shutdown();

    return 0;
}

glm::vec3 getFrontVector(float yaw, float pitch)
{
    return glm::rotateY(glm::rotateX(glm::vec3(0, 0, 1), glm::radians(pitch)), glm::radians(yaw));
}
