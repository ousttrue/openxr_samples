#pragma once
#include "android_native_app_glue.h"
#include <stdint.h>

struct engine {
  class EngineImpl *impl_ = nullptr;
  engine(const engine &) = delete;
  engine &operator=(const engine &) = delete;
  engine();
  ~engine();
};

void OnAppCmd(struct android_app *app, int32_t cmd);
