/* ------------------------------------------------ *
 * The MIT License (MIT)
 * Copyright (c) 2019 terryky1220@gmail.com
 * ------------------------------------------------ */
#include "util_render_target.h"
#include "assertgl.h"
#include "util_egl.h"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define UNUSED(x) (void)(x)

int render_target::create_render_target(int w, int h, unsigned int flags) {
  /* texture for color */
  GLuint tex_c = 0;
  if (flags & RTARGET_COLOR) {
    glGenTextures(1, &tex_c);
    glBindTexture(GL_TEXTURE_2D, tex_c);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 0);
  }

  /* texture for depth */
  GLuint tex_z = 0;
  if (flags & RTARGET_DEPTH) {
    glGenTextures(1, &tex_z);
    glBindTexture(GL_TEXTURE_2D, tex_z);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, w, h, 0,
                 GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL);
  }

  glBindTexture(GL_TEXTURE_2D, 0);

  GLuint fbo = 0;
  if (flags) {
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           tex_c, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           tex_z, 0);
  }

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  this->texc_id = tex_c;
  this->texz_id = tex_z;
  this->fbo_id = fbo;
  this->width = w;
  this->height = h;

  GLASSERT();

  return 0;
}

int render_target::destroy_render_target() {
  glDeleteTextures(1, &this->texc_id);
  glDeleteTextures(1, &this->texz_id);
  glDeleteFramebuffers(1, &this->fbo_id);

  this->texc_id = 0;
  this->texz_id = 0;
  this->fbo_id = 0;
  this->width = 0;
  this->height = 0;

  GLASSERT();

  return 0;
}

int render_target::set_render_target() {
  if (fbo_id) {
    glBindFramebuffer(GL_FRAMEBUFFER, this->fbo_id);
    glViewport(0, 0, this->width, this->height);
    glScissor(0, 0, this->width, this->height);
  } else {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    int w, h;
    egl_get_current_surface_dimension(&w, &h);
    glViewport(0, 0, w, h);
    glScissor(0, 0, w, h);
  }

  GLASSERT();

  return 0;
}

int render_target::get_render_target() {

  this->texc_id = 0;
  this->texz_id = 0;
  this->fbo_id = 0;
  this->width = 0;
  this->height = 0;

  GLint tex_c, tex_z, fbo;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
  if (fbo > 0) {
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                          GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
                                          &tex_c);

    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                                          GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,
                                          &tex_z);
    this->fbo_id = fbo;
    this->texc_id = tex_c;
    this->texz_id = tex_z;
  }

  int viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  this->width = viewport[2];
  this->height = viewport[3];

  GLASSERT();

  return 0;
}
