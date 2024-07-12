#include "uniforms/light_uniform.hpp"
#include "pass_resource/light_data.hpp"

#include "vgfw.hpp"

void uploadLightUniform(FrameGraph& fg, FrameGraphBlackboard& blackboard, const DirectionalLight& light)
{
    blackboard.add<LightData>() = fg.addCallbackPass<LightData>(
        "Upload LightUniform",
        [&](FrameGraph::Builder& builder, LightData& data) {
            data.lightUniform = builder.create<vgfw::renderer::framegraph::FrameGraphBuffer>(
                "LightUniform", {.size = sizeof(DirectionalLight)});
            data.lightUniform = builder.write(data.lightUniform);
        },
        [=](const LightData& data, FrameGraphPassResources& resources, void* ctx) {
            static_cast<vgfw::renderer::RenderContext*>(ctx)->upload(
                vgfw::renderer::framegraph::getBuffer(resources, data.lightUniform),
                0,
                sizeof(DirectionalLight),
                &light);
        });
}