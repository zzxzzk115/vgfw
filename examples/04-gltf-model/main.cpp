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

layout(binding = 0) uniform sampler2D baseColor;
layout(binding = 1) uniform sampler2D metallicRoughness;

layout(location = 3) uniform vec3 lightPos;
layout(location = 4) uniform vec3 viewPos;
layout(location = 5) uniform vec3 lightColor;
layout(location = 6) uniform vec3 objectColor;
layout(location = 7) uniform float lightIntensity;


// Cook-Torrance GGX (Trowbridge-Reitz) Distribution
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.1415926535897932384626433832795 * denom * denom;

    return num / max(denom, 0.001); // prevent divide by zero for roughness=0.0 and NdotH=1.0
}

float GeometrySmith_GGX(float NdotX, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;

    float num = NdotX;
    float denom = NdotX * (1.0 - a) + a;

    return num / denom;
}

// Smith's GGX Visibility Function (Schlick-Beckmann approximation)
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySmith_GGX(NdotV, roughness);
    float ggx1 = GeometrySmith_GGX(NdotL, roughness);

    return ggx1 * ggx2;
}

// Schlick's approximation for the Fresnel term
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    vec2 uv = vec2(vTexCoords.x, 1.0 - vTexCoords.y);

    // Retrieve material properties from metallicRoughness texture
    vec4 texSample = texture(metallicRoughness, uv);
    float metallic = texSample.b;
    float roughness = texSample.g;

    // Ambient
    vec3 ambient = lightIntensity * lightColor * 0.03;

    // Diffuse
    vec3 norm = normalize(vNormal); // Use vertex normal directly
    vec3 lightDir = normalize(lightPos - vFragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = lightIntensity * diff * lightColor;

    // Specular (Cook-Torrance BRDF)
    vec3 viewDir = normalize(viewPos - vFragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);

    float NDF = DistributionGGX(norm, halfwayDir, roughness);
    float G = GeometrySmith(norm, viewDir, lightDir, roughness);
    vec3 F0 = vec3(0.04); // default specular reflectance
    vec3 F = fresnelSchlick(max(dot(halfwayDir, viewDir), 0.0), F0);
    vec3 specular = (NDF * G * F) / (4.0 * max(dot(norm, viewDir), 0.0) * max(dot(norm, lightDir), 0.0));

    // Combine ambient, diffuse, and specular components
    vec3 result = (ambient + (1.0 - metallic) * diffuse + metallic * specular) * objectColor;

    // Output final color with baseColor texture
    FragColor = texture(baseColor, uv) * vec4(result, 1.0);
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
    auto window = vgfw::window::create({.title = "04-gltf-model", .enableMSAA = true, .aaSample = 8});

    // Init renderer
    vgfw::renderer::init({.window = window});

    // Load model
    vgfw::resource::Model suzanneModel {};
    if (!vgfw::io::load("assets/models/Suzanne.gltf", suzanneModel))
    {
        return -1;
    }

    // Get graphics & render context
    auto& rc = vgfw::renderer::getRenderContext();

    // Get vertex array object
    auto vao = rc.getVertexArray(suzanneModel.meshPrimitives[0].vertexFormat->getAttributes());

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

    // Get textures
    auto* baseColorTexture =
        suzanneModel
            .textureMap[suzanneModel.materialMap[suzanneModel.meshPrimitives[0].materialIndex].baseColorTextureIndex];
    auto* metallicRoughnessTexture =
        suzanneModel.textureMap[suzanneModel.materialMap[suzanneModel.meshPrimitives[0].materialIndex]
                                    .metallicRoughnessTextureIndex];

    // Start time
    auto startTime = std::chrono::high_resolution_clock::now();

    // Camera properties
    float     fov = 60.0f;
    glm::vec3 viewPos(0.0f, 0.0f, 3.0f);

    // Light properties
    glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
    glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
    glm::vec3 objectColor(1.0f, 1.0f, 1.0f);
    float     lightIntensity = 20.0f;

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
        glm::mat4 view = glm::lookAt(viewPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

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
            .setUniformVec3("lightPos", lightPos)
            .setUniformVec3("viewPos", viewPos)
            .setUniformVec3("lightColor", lightColor)
            .setUniformVec3("objectColor", objectColor)
            .setUniform1f("lightIntensity", lightIntensity)
            .bindTexture(0, *baseColorTexture)
            .bindTexture(1, *metallicRoughnessTexture);

        suzanneModel.meshPrimitives[0].draw(rc);

        vgfw::renderer::beginImGui();
        ImGui::Begin("GLTF Model");
        ImGui::SliderFloat("Camera FOV", &fov, 1.0f, 179.0f);
        ImGui::DragFloat3("Camera Position", glm::value_ptr(viewPos));
        ImGui::DragFloat("Light Intensity", &lightIntensity);
        ImGui::DragFloat3("Light Position", glm::value_ptr(lightPos));
        ImGui::ColorEdit3("Light Color", glm::value_ptr(lightColor));
        ImGui::ColorEdit3("Object Color", glm::value_ptr(objectColor));
        ImGui::End();
        vgfw::renderer::endImGui();

        vgfw::renderer::present();
    }

    // Cleanup
    vgfw::shutdown();

    return 0;
}