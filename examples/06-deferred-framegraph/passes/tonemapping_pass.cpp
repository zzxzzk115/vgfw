#include "passes/tonemapping_pass.hpp"

TonemappingPass::TonemappingPass(vgfw::renderer::RenderContext& rc) : BasePass(rc)
{
    auto program = m_RenderContext.createGraphicsProgram(vgfw::utils::readFileAllText("shaders/fullscreen.vert"),
                                                         vgfw::utils::readFileAllText("shaders/tonemapping.frag"));

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

TonemappingPass::~TonemappingPass() { m_RenderContext.destroy(m_Pipeline); }

FrameGraphResource TonemappingPass::addToGraph(FrameGraph& fg, FrameGraphResource input)
{
    const auto extent = fg.getDescriptor<vgfw::renderer::framegraph::FrameGraphTexture>(input).extent;

    struct Data
    {
        FrameGraphResource output;
    };
    const auto& pass = fg.addCallbackPass<Data>(
        "Tone-mapping Pass",
        [&](FrameGraph::Builder& builder, Data& data) {
            builder.read(input);

            data.output = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "Tone-mapped SceneColor", {.extent = extent, .format = vgfw::renderer::PixelFormat::eRGB8_UNorm});
            data.output = builder.write(data.output);
        },
        [=, this](const Data& data, FrameGraphPassResources& resources, void* ctx) {
            NAMED_DEBUG_MARKER("Tone-mapping Pass");
            VGFW_PROFILE_GL("Tone-mapping Pass");
            VGFW_PROFILE_NAMED_SCOPE("Tone-mapping Pass");

            const vgfw::renderer::RenderingInfo renderingInfo {
                .area             = {.extent = extent},
                .colorAttachments = {{
                    .image = vgfw::renderer::framegraph::getTexture(resources, data.output),
                }},
            };

            auto&      rc          = *static_cast<vgfw::renderer::RenderContext*>(ctx);
            const auto framebuffer = rc.beginRendering(renderingInfo);
            rc.bindGraphicsPipeline(m_Pipeline)
                .bindTexture(0, vgfw::renderer::framegraph::getTexture(resources, input))
                .drawFullScreenTriangle()
                .endRendering(framebuffer);
        });

    return pass.output;
}