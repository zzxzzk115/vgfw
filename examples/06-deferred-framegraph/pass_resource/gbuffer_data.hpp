#pragma once

#include <fg/Fwd.hpp>

struct GBufferData
{
    FrameGraphResource position;
    FrameGraphResource normal;
    FrameGraphResource albedo;
    FrameGraphResource emissive;
    FrameGraphResource metallicRoughnessAO;
    FrameGraphResource depth;
};