#pragma once
#include "util_render_target.h"
#include "util_shader.h"
#include <openxr/openxr.h>

class Renderer {
  shader_obj_t s_sobj;

public:
  void init_gles_scene();
  int render_gles_scene(XrCompositionLayerProjectionView &layerView,
                        render_target &rtarget, const XrPosef &stagePose);

private:
  int draw_triangle(float *matStage);
  int draw_stage(float *matStage);
  int draw_line(float *mtxPV, float *p0, float *p1, float *color);
};
