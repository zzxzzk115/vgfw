#pragma once

#include "base_pass.hpp"

class DeferredLightingPass : public BasePass
{
public:
    explicit DeferredLightingPass(vgfw::renderer::RenderContext& rc);
    ~DeferredLightingPass();

    FrameGraphResource addToGraph(FrameGraph& fg, FrameGraphBlackboard& blackboard);

private:
    vgfw::renderer::GraphicsPipeline m_Pipeline;
};