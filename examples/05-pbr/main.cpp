#define VGFW_IMPLEMENTATION
#include "vgfw.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

struct DirectionalLight
{
    glm::vec3 direction = glm::normalize(glm::vec3(1.0f, -1.0f, 0.0f));
    float     intensity = 0.5f;
    glm::vec3 color     = {1, 1, 1};
};

struct Camera
{
    struct CameraUniform
    {
        alignas(16) glm::vec3 position;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 projection;
    } data;

    float fov   = 60.0f;
    float zNear = 1.0f;
    float zFar  = 10000.0f;

    float yaw = 0, pitch = 0;
    float speed = 250, sensitivity = 40;
    bool  isCaptureCursor = false;

    void updateData(const std::shared_ptr<vgfw::window::Window>& window);
    void update(const std::shared_ptr<vgfw::window::Window>& window, float dt);
};

void uploadCamera(const std::shared_ptr<vgfw::renderer::Buffer>& cameraBuffer,
                  const Camera::CameraUniform&                   cameraUniform,
                  vgfw::renderer::RenderContext&                 rc);

void uploadLight(const std::shared_ptr<vgfw::renderer::Buffer>& lightBuffer,
                 const DirectionalLight&                        light,
                 vgfw::renderer::RenderContext&                 rc);

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

    DirectionalLight light {};

    auto lightBuf    = rc.createBuffer(sizeof(DirectionalLight), &light);
    auto lightBuffer = std::shared_ptr<vgfw::renderer::Buffer>(new vgfw::renderer::Buffer {std::move(lightBuf)},
                                                               vgfw::renderer::RenderContext::ResourceDeleter {rc});

    // Camera properties
    Camera camera {};
    camera.data.position = {-1150, 200, -45};
    camera.yaw           = 90.0f;

    auto cameraBuf    = rc.createBuffer(sizeof(Camera::CameraUniform), &camera.data);
    auto cameraBuffer = std::shared_ptr<vgfw::renderer::Buffer>(new vgfw::renderer::Buffer {std::move(cameraBuf)},
                                                                vgfw::renderer::RenderContext::ResourceDeleter {rc});
    camera.updateData(window);

    // Create a texture sampler
    auto sampler = rc.createSampler({.maxAnisotropy = 8});

    vgfw::time::TimePoint lastTime = vgfw::time::Clock::now();

    // Main loop
    while (!window->shouldClose())
    {
        vgfw::time::TimePoint currentTime = vgfw::time::Clock::now();
        vgfw::time::Duration  deltaTime   = currentTime - lastTime;
        lastTime                          = currentTime;

        float deltaTimeSeconds = deltaTime.count();

        window->onTick();

        camera.update(window, deltaTimeSeconds);

        uploadCamera(cameraBuffer, camera.data, rc);
        uploadLight(lightBuffer, light, rc);

        vgfw::renderer::beginFrame();

        // Render
        rc.beginRendering({.extent = {.width = window->getWidth(), .height = window->getHeight()}},
                          glm::vec4 {0.2f, 0.3f, 0.3f, 1.0f},
                          1.0f);

        for (auto& meshPrimitive : sponza.meshPrimitives)
        {
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
                .bindUniformBuffer(0, *cameraBuffer)
                .bindUniformBuffer(1, *lightBuffer)
                .bindMeshPrimitiveMaterialBuffer(2, meshPrimitive)
                .bindMeshPrimitiveTextures(0, meshPrimitive, sampler)
                .drawMeshPrimitive(meshPrimitive);
        }

        ImGui::Begin("PBR");
        ImGui::SliderFloat("Camera FOV", &camera.fov, 1.0f, 179.0f);
        ImGui::Text("Press CAPSLOCK to toggle the camera (W/A/S/D/Q/E + Mouse)");
        ImGui::End();

        vgfw::renderer::endFrame();

        vgfw::renderer::present();
    }

    // Cleanup
    vgfw::shutdown();

    return 0;
}

void Camera::updateData(const std::shared_ptr<vgfw::window::Window>& window)
{
    auto direction  = glm::rotateY(glm::rotateX(glm::vec3(0, 0, 1), glm::radians(pitch)), glm::radians(yaw));
    data.view       = glm::lookAt(data.position, data.position + direction, glm::vec3(.0f, 1.0f, .0f));
    data.projection = glm::perspective(glm::radians(fov), window->getWidth() * 1.0f / window->getHeight(), zNear, zFar);
}

void Camera::update(const std::shared_ptr<vgfw::window::Window>& window, float dt)
{
    auto*         glfwWindow = reinterpret_cast<GLFWwindow*>(window->getPlatformWindow());
    static double lastX, lastY;
    static bool   first = true;
    double        xpos, ypos;
    glfwGetCursorPos(glfwWindow, &xpos, &ypos);
    if (first)
    {
        first = false;
        lastX = xpos;
        lastY = ypos;
        return;
    }
    static bool isCapslockDown = false;
    auto        capslock       = glfwGetKey(glfwWindow, GLFW_KEY_CAPS_LOCK);
    if (!isCapslockDown && capslock == GLFW_PRESS)
    {
        isCapslockDown = true;
    }
    else if (isCapslockDown && capslock == GLFW_RELEASE)
    {
        isCapslockDown  = false;
        isCaptureCursor = !isCaptureCursor;
        glfwSetInputMode(glfwWindow, GLFW_CURSOR, isCaptureCursor ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    }
    if (!isCaptureCursor)
    {
        lastX = xpos;
        lastY = ypos;
        return;
    }

    double deltaX = (xpos - lastX) * dt * sensitivity;
    double deltaY = (ypos - lastY) * dt * sensitivity;

    yaw -= deltaX;
    pitch = std::clamp<float>(pitch + deltaY, -89.0f, 89.0f);
    lastX = xpos;
    lastY = ypos;

    auto  d    = glm::rotateY(glm::vec3(0.0f, 0.0f, 1.0f), glm::radians(yaw));
    auto  cd   = glm::rotateY(d, glm::radians(90.0f));
    float move = speed * dt;
    if (glfwGetKey(glfwWindow, GLFW_KEY_W))
        data.position += d * move;
    if (glfwGetKey(glfwWindow, GLFW_KEY_S))
        data.position -= d * move;
    if (glfwGetKey(glfwWindow, GLFW_KEY_A))
        data.position += cd * move;
    if (glfwGetKey(glfwWindow, GLFW_KEY_D))
        data.position -= cd * move;
    if (glfwGetKey(glfwWindow, GLFW_KEY_Q))
        data.position.y += move;
    if (glfwGetKey(glfwWindow, GLFW_KEY_E))
        data.position.y -= move;

    updateData(window);
}

glm::vec3 getFrontVector(float yaw, float pitch)
{
    return glm::rotateY(glm::rotateX(glm::vec3(0, 0, 1), glm::radians(pitch)), glm::radians(yaw));
}

void uploadLight(const std::shared_ptr<vgfw::renderer::Buffer>& lightBuffer,
                 const DirectionalLight&                        light,
                 vgfw::renderer::RenderContext&                 rc)
{
    rc.upload(*lightBuffer, 0, sizeof(DirectionalLight), &light);
}

void uploadCamera(const std::shared_ptr<vgfw::renderer::Buffer>& cameraBuffer,
                  const Camera::CameraUniform&                   cameraUniform,
                  vgfw::renderer::RenderContext&                 rc)
{
    rc.upload(*cameraBuffer, 0, sizeof(Camera::CameraUniform), &cameraUniform);
}