#pragma once
#include "util_shader.h"
#include <memory>
#include <openxr/openxr.h>
#include <stdint.h>
#include <vector>

class Renderer {
  shader_obj_t s_sobj;

public:
  void init_gles_scene();
  int render_gles_scene(const XrPosef &stagePose, uint32_t fbo, int x, int y,
                        int w, int h, const XrFovf &fov,
                        const XrPosef &viewPose);

private:
  int draw_triangle(float *matStage);
  int draw_stage(float *matStage);
  int draw_line(float *mtxPV, float *p0, float *p1, float *color);
};
