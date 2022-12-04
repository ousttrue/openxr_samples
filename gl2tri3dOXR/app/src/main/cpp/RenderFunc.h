#pragma once
#include <functional>
#include <openxr/openxr.h>
#include <stdint.h>

using RenderFunc = std::function<int(
    const XrPosef &stagePose, uint32_t fbo, int x, int y, int w, int h,
    const XrFovf &fov, const XrPosef &viewPose)>;
