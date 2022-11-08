#pragma once
#include "android_native_app_glue.h"
#include <stdint.h>
#include <EGL/egl.h>

class EventHandler {
  EGLDisplay display_ = 0;
  EGLSurface surface_ = 0;
  EGLint width_ = 0;
  EGLint height_ = 0;
  class Renderer *renderer_ = nullptr;

public:
  EventHandler(const EventHandler &) = delete;
  EventHandler &operator=(const EventHandler &) = delete;
  EventHandler();
  ~EventHandler();
  void bind(struct android_app *state);
  static void OnAppCmd(struct android_app *state, int32_t cmd);

private:
  void onAppCmd(struct android_app *state, int32_t cmd);
  bool initEGL(struct android_app *state);
};
