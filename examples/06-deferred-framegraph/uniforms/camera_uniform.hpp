#pragma once

#include "camera.hpp"

#include <fg/Fwd.hpp>

void uploadCameraUniform(FrameGraph& fg, FrameGraphBlackboard& blackboard, const Camera::CameraUniform& cameraUniform);