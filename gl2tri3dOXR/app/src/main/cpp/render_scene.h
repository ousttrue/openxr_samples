#pragma once
#include "util_render_target.h"
#include "util_shader.h"
#include <openxr/openxr.h>
#include <stdint.h>
#include <vector>

struct Swapchain {
  std::vector<std::shared_ptr<render_target>> rtarget_array;
};

class Renderer {
  shader_obj_t s_sobj;
  std::vector<Swapchain> swapchains;

public:
  void init_gles_scene();
  int render_gles_scene(size_t viewIndex, size_t swapchainIndex, int x, int y,
                        int w, int h, const XrFovf &fov, const XrPosef &pose,
                        const XrPosef &stagePose);

  bool createBackbuffer(size_t viewIndex, size_t swapchainIndex, uint32_t tex_c, int width, int height);

private:
  uint32_t getFBO(size_t viewIndex, size_t swapchainIndex) const;
  int draw_triangle(float *matStage);
  int draw_stage(float *matStage);
  int draw_line(float *mtxPV, float *p0, float *p1, float *color);
};
