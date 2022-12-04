#pragma once
#include "RenderFunc.h"

class XrApp {

  struct XrAppImpl *impl_ = nullptr;

public:
  XrApp();
  ~XrApp();
  bool CreateInstance(struct android_app *app);
  bool CreateGraphics();
  bool CreateSession();
  bool IsSessionRunning() const;
  void Render(const RenderFunc &func);
};
