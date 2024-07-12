#include "camera.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

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
