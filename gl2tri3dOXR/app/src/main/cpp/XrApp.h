#pragma once
#include "render_interface.h"
#include <functional>

using RenderFunc =
    std::function<void(unsigned int, const render_interface::ViewInfo)>;

class XrApp {

  struct XrAppImpl *impl_ = nullptr;

public:
  XrApp();
  ~XrApp();
  bool CreateInstance(struct android_app *app);
  bool CreateSession_EGL(int major, int minor);
  bool IsSessionRunning() const;
  void Render(const RenderFunc &func);
};
