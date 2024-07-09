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
    auto window = vgfw::window::create({.title = "05-pbr", .aaSample = vgfw::window::AASample::e8});

    // Init renderer
    vgfw::renderer::init({.window = window});

    // Get graphics & render context
    auto& rc = vgfw::renderer::getRenderContext();

    // Load model
    vgfw::resource::Model sponza {};
    if (!vgfw::io::load("assets/models/Sponza/glTF/Sponza.gltf", sponza, rc))
    {
        return -1;
    }

    // Create shader program
    auto program = rc.createGraphicsProgram(vgfw::utils::readFileAllText("shaders/default.vert"),
                                            vgfw::utils::readFileAllText("shaders/default.frag"));

    // Camera properties
    glm::vec3 cameraPos(-1150, 200, -45);
    float     fov   = 60.0f;
    float     yaw   = 90.0f;
    float     pitch = 0.0f;

    // Main loop
    while (!window->shouldClose())
    {
        window->onTick();

        // Create the view matrix
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + getFrontVector(yaw, pitch), glm::vec3(0.0f, 1.0f, 0.0f));

        // Create the projection matrix
        glm::mat4 projection =
            glm::perspective(glm::radians(fov), window->getWidth() * 1.0f / window->getHeight(), 1.0f, 10000.0f);

        // Render
        rc.beginRendering({.extent = {.width = window->getWidth(), .height = window->getHeight()}},
                          glm::vec4 {0.2f, 0.3f, 0.3f, 1.0f},
                          1.0f);

        for (uint32_t primitiveIndex = 0; primitiveIndex < sponza.meshPrimitives.size(); ++primitiveIndex)
        {
            auto& meshPrimitive = sponza.meshPrimitives[primitiveIndex];

            auto vao = rc.getVertexArray(meshPrimitive.vertexFormat->getAttributes());

            // Build a graphics pipeline
            auto graphicsPipeline = vgfw::renderer::GraphicsPipeline::Builder {}
                                        .setDepthStencil({
                                            .depthTest      = true,
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

            rc.bindGraphicsPipeline(graphicsPipeline)
                .setUniformMat4("view", view)
                .setUniformMat4("projection", projection)
                .setUniformVec3("viewPos", cameraPos)
                .bindUniformBuffer(0, *meshPrimitive.materialBuffer);

            sponza.bindMeshPrimitiveTextures(primitiveIndex, 1, rc);

            meshPrimitive.draw(rc);
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
