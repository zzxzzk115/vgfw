#include "passes/deferred_lighting_pass.hpp"
#include "pass_resource/camera_data.hpp"
#include "pass_resource/gbuffer_data.hpp"
#include "pass_resource/light_data.hpp"

DeferredLightingPass::DeferredLightingPass(vgfw::renderer::RenderContext& rc) : BasePass(rc)
{
    auto program =
        m_RenderContext.createGraphicsProgram(vgfw::utils::readFileAllText("shaders/fullscreen.vert"),
                                              vgfw::utils::readFileAllText("shaders/deferred_lighting.frag"));

    m_Pipeline = vgfw::renderer::GraphicsPipeline::Builder {}
                     .setShaderProgram(program)
                     .setDepthStencil({
                         .depthTest  = false,
                         .depthWrite = false,
                     })
                     .setRasterizerState({
                         .polygonMode = vgfw::renderer::PolygonMode::eFill,
                         .cullMode    = vgfw::renderer::CullMode::eBack,
                         .scissorTest = false,
                     })
                     .build();
}

DeferredLightingPass::~DeferredLightingPass() { m_RenderContext.destroy(m_Pipeline); }

FrameGraphResource DeferredLightingPass::addToGraph(FrameGraph& fg, FrameGraphBlackboard& blackboard)
{
    const auto [cameraUniform] = blackboard.get<CameraData>();
    const auto [lightUniform]  = blackboard.get<LightData>();

    const auto& gBuffer = blackboard.get<GBufferData>();

    const auto extent = fg.getDescriptor<vgfw::renderer::framegraph::FrameGraphTexture>(gBuffer.depth).extent;

    struct Data
    {
        FrameGraphResource sceneColorHDR;
    };
    const auto& deferredLighting = fg.addCallbackPass<Data>(
        "Deferred Lighting Pass",
        [&](FrameGraph::Builder& builder, Data& data) {
            builder.read(cameraUniform);
            builder.read(lightUniform);

            builder.read(gBuffer.position);
            builder.read(gBuffer.normal);
            builder.read(gBuffer.albedo);
            builder.read(gBuffer.emissive);
            builder.read(gBuffer.metallicRoughnessAO);

            data.sceneColorHDR = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "SceneColorHDR", {.extent = extent, .format = vgfw::renderer::PixelFormat::eRGB16F});
            data.sceneColorHDR = builder.write(data.sceneColorHDR);
        },
        [=, this](const Data& data, FrameGraphPassResources& resources, void* ctx) {
            auto& rc = *static_cast<vgfw::renderer::RenderContext*>(ctx);

            const vgfw::renderer::RenderingInfo renderingInfo {
                .area             = {.extent = extent},
                .colorAttachments = {{
                    .image      = vgfw::renderer::framegraph::getTexture(resources, data.sceneColorHDR),
                    .clearValue = glm::vec4 {0.0f},
                }},
            };

            const auto framebuffer = rc.beginRendering(renderingInfo);

            rc.bindGraphicsPipeline(m_Pipeline)
                .bindUniformBuffer(0, vgfw::renderer::framegraph::getBuffer(resources, cameraUniform))
                .bindUniformBuffer(1, vgfw::renderer::framegraph::getBuffer(resources, lightUniform))
                .bindTexture(0, vgfw::renderer::framegraph::getTexture(resources, gBuffer.position))
                .bindTexture(1, vgfw::renderer::framegraph::getTexture(resources, gBuffer.normal))
                .bindTexture(2, vgfw::renderer::framegraph::getTexture(resources, gBuffer.albedo))
                .bindTexture(3, vgfw::renderer::framegraph::getTexture(resources, gBuffer.emissive))
                .bindTexture(4, vgfw::renderer::framegraph::getTexture(resources, gBuffer.metallicRoughnessAO))
                .drawFullScreenTriangle()
                .endRendering(framebuffer);
        });

    return deferredLighting.sceneColorHDR;
}