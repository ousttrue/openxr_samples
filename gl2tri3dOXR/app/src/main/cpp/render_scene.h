#pragma once
#include "render_interface.h"
#include "util_shader.h"

class Renderer {
  shader_obj_t s_sobj;

public:
  void init_gles_scene(int *major, int *minor);
  void render_gles_scene(unsigned int fbo,
                         const render_interface::ViewInfo &info);

private:
  int draw_triangle(float *matStage);
  int draw_stage(float *matStage);
  int draw_line(float *mtxPV, float *p0, float *p1, float *color);
};
