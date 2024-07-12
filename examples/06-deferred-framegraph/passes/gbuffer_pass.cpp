#include "passes/gbuffer_pass.hpp"
#include "pass_resource/camera_data.hpp"
#include "pass_resource/gbuffer_data.hpp"

GBufferPass::GBufferPass(vgfw::renderer::RenderContext& rc) : BasePass(rc) {}

GBufferPass::~GBufferPass()
{
    for (auto& [_, pipeline] : m_Pipelines)
    {
        m_RenderContext.destroy(pipeline);
    }
}

void GBufferPass::addToGraph(FrameGraph&                                       fg,
                             FrameGraphBlackboard&                             blackboard,
                             const vgfw::renderer::Extent2D&                   resolution,
                             const std::vector<vgfw::resource::MeshPrimitive>& meshPrimitives)
{
    const auto [cameraUniform] = blackboard.get<CameraData>();

    blackboard.add<GBufferData>() = fg.addCallbackPass<GBufferData>(
        "GBuffer Pass",
        [&, resolution](FrameGraph::Builder& builder, GBufferData& data) {
            builder.read(cameraUniform);

            data.position = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "Position", {.extent = resolution, .format = vgfw::renderer::PixelFormat::eRGB16F});
            data.position = builder.write(data.position);

            data.normal = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "Normal", {.extent = resolution, .format = vgfw::renderer::PixelFormat::eRGB16F});
            data.normal = builder.write(data.normal);

            data.albedo = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "Albedo", {.extent = resolution, .format = vgfw::renderer::PixelFormat::eRGB8_UNorm});
            data.albedo = builder.write(data.albedo);

            data.emissive = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "Emissive", {.extent = resolution, .format = vgfw::renderer::PixelFormat::eRGB8_UNorm});
            data.emissive = builder.write(data.emissive);

            data.metallicRoughnessAO = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "Metallic Roughness AO", {.extent = resolution, .format = vgfw::renderer::PixelFormat::eRGB8_UNorm});
            data.metallicRoughnessAO = builder.write(data.metallicRoughnessAO);

            data.depth = builder.create<vgfw::renderer::framegraph::FrameGraphTexture>(
                "Depth", {.extent = resolution, .format = vgfw::renderer::PixelFormat::eDepth32F});
            data.depth = builder.write(data.depth);
        },
        [=, &meshPrimitives, this](const GBufferData& data, FrameGraphPassResources& resources, void* ctx) {
            NAMED_DEBUG_MARKER("GBuffer Pass");
            VGFW_PROFILE_GL("GBuffer Pass");
            VGFW_PROFILE_NAMED_SCOPE("GBuffer Pass");

            auto& rc = *static_cast<vgfw::renderer::RenderContext*>(ctx);

            constexpr glm::vec4 kBlackColor {0.0f};
            constexpr float     kFarPlane {1.0f};

            vgfw::renderer::RenderingInfo renderingInfo = {
                .area = {.extent = resolution},
                .colorAttachments =
                    {
                        {.image      = vgfw::renderer::framegraph::getTexture(resources, data.position),
                         .clearValue = kBlackColor},
                        {.image      = vgfw::renderer::framegraph::getTexture(resources, data.normal),
                         .clearValue = kBlackColor},
                        {.image      = vgfw::renderer::framegraph::getTexture(resources, data.albedo),
                         .clearValue = kBlackColor},
                        {.image      = vgfw::renderer::framegraph::getTexture(resources, data.emissive),
                         .clearValue = kBlackColor},
                        {.image      = vgfw::renderer::framegraph::getTexture(resources, data.metallicRoughnessAO),
                         .clearValue = kBlackColor},
                    },
                .depthAttachment = vgfw::renderer::AttachmentInfo {
                    .image = vgfw::renderer::framegraph::getTexture(resources, data.depth), .clearValue = kFarPlane}};

            auto frameBuffer = rc.beginRendering(renderingInfo);

            // Draw
            for (const auto& meshPrimitive : meshPrimitives)
            {
                rc.bindGraphicsPipeline(getPipeline(*meshPrimitive.vertexFormat))
                    .bindUniformBuffer(0, vgfw::renderer::framegraph::getBuffer(resources, cameraUniform))
                    .bindMeshPrimitiveMaterialBuffer(1, meshPrimitive)
                    .bindMeshPrimitiveTextures(0, meshPrimitive)
                    .drawMeshPrimitive(meshPrimitive);
            }

            rc.endRendering(frameBuffer);
        });
}

vgfw::renderer::GraphicsPipeline& GBufferPass::getPipeline(const vgfw::renderer::VertexFormat& vertexFormat)
{
    size_t hash = vertexFormat.getHash();

    vgfw::renderer::GraphicsPipeline* passPipeline = nullptr;

    if (const auto it = m_Pipelines.find(hash); it != m_Pipelines.cend())
        passPipeline = &it->second;

    if (!passPipeline)
    {
        auto pipeline = createPipeline(vertexFormat);

        const auto& it = m_Pipelines.insert_or_assign(hash, std::move(pipeline)).first;
        passPipeline   = &it->second;
    }

    return *passPipeline;
}

vgfw::renderer::GraphicsPipeline GBufferPass::createPipeline(const vgfw::renderer::VertexFormat& vertexFormat)
{
    auto vertexArrayObject = m_RenderContext.getVertexArray(vertexFormat.getAttributes());

    auto program = m_RenderContext.createGraphicsProgram(vgfw::utils::readFileAllText("shaders/geometry.vert"),
                                                         vgfw::utils::readFileAllText("shaders/gbuffer.frag"));

    return vgfw::renderer::GraphicsPipeline::Builder {}
        .setDepthStencil({
            .depthTest      = true,
            .depthWrite     = true,
            .depthCompareOp = vgfw::renderer::CompareOp::eLessOrEqual,
        })
        .setRasterizerState({
            .polygonMode = vgfw::renderer::PolygonMode::eFill,
            .cullMode    = vgfw::renderer::CullMode::eBack,
            .scissorTest = false,
        })
        .setVAO(vertexArrayObject)
        .setShaderProgram(program)
        .build();
}