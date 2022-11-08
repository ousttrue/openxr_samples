#pragma once
#include <openxr/openxr.h>
#include <stdint.h>
#include <vector>

struct ViewSurface {
  uint32_t width;
  uint32_t height;
  XrSwapchain swapchain;
};

class OpenXRApp {
  XrInstance instance_ = {0};
  XrSystemId systemId_ = {0};
  XrSession session_ = {0};
  XrSpace appSpace_ = {0};
  XrSpace stageSpace_ = {0};
  bool sessionRunning_;

public:
  std::vector<ViewSurface> viewSwapchains_;
  OpenXRApp();
  ~OpenXRApp();
  void createInstance(struct android_app *app);
  bool confirmGLES(int major, int minor);
  void createSession();
  bool isSessionRunning() const { return sessionRunning_; }

  int64_t beginFrame();
  void endFrame(int64_t time);
};
