#pragma once

#include "light.hpp"

#include <fg/Fwd.hpp>

void uploadLightUniform(FrameGraph& fg, FrameGraphBlackboard& blackboard, const DirectionalLight& light);