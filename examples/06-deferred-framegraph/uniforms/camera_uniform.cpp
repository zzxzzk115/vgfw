#include "uniforms/camera_uniform.hpp"
#include "pass_resource/camera_data.hpp"

#include "vgfw.hpp"

void uploadCameraUniform(FrameGraph& fg, FrameGraphBlackboard& blackboard, const Camera::CameraUniform& cameraUniform)
{
    blackboard.add<CameraData>() = fg.addCallbackPass<CameraData>(
        "Upload CameraUniform",
        [&](FrameGraph::Builder& builder, CameraData& data) {
            data.cameraUniform = builder.create<vgfw::renderer::framegraph::FrameGraphBuffer>(
                "CameraUniform", {.size = sizeof(Camera::CameraUniform)});
            data.cameraUniform = builder.write(data.cameraUniform);
        },
        [=](const CameraData& data, FrameGraphPassResources& resources, void* ctx) {
            static_cast<vgfw::renderer::RenderContext*>(ctx)->upload(
                vgfw::renderer::framegraph::getBuffer(resources, data.cameraUniform),
                0,
                sizeof(Camera::CameraUniform),
                &cameraUniform);
        });
}