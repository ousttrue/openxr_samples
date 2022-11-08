#pragma once
#include <openxr/openxr.h>
#include "util_render_target.h"

int init_gles_scene ();
int render_gles_scene (XrCompositionLayerProjectionView &layerView,
                       render_target_t &rtarget, const XrPosef &stagePose);
