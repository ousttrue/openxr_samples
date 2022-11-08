/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2019 terryky1220@gmail.com
 * ------------------------------------------------ */
#pragma once
#include <GLES/gl.h>

#define RTARGET_DEFAULT (0 << 0)
#define RTARGET_COLOR (1 << 0)
#define RTARGET_DEPTH (1 << 1)

struct render_target {
  GLuint texc_id = 0; /* color */
  GLuint texz_id = 0; /* depth */
  GLuint fbo_id = 0;
  int width = 0;
  int height = 0;

  int create_render_target(int w, int h, unsigned int flags);
  int destroy_render_target();
  int set_render_target();
  int get_render_target();
  int blit_render_target(int x, int y, int w, int h);
};
