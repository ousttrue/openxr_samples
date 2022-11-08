#pragma once
// #include "android_native_app_glue.h"
// #include <stdint.h>
#include <EGL/egl.h>

class EGLState {
  EGLDisplay display_ = 0;
  EGLSurface surface_ = 0;
  EGLContext context_ = 0;
  EGLint width_ = 0;
  EGLint height_ = 0;

public:
  bool initialize(struct android_app *state);
};
