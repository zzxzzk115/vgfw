#pragma once

#include "base_pass.hpp"

class TonemappingPass : public BasePass
{
public:
    explicit TonemappingPass(vgfw::renderer::RenderContext& rc);
    ~TonemappingPass();

    FrameGraphResource addToGraph(FrameGraph& fg, FrameGraphResource input);

private:
    vgfw::renderer::GraphicsPipeline m_Pipeline;
};