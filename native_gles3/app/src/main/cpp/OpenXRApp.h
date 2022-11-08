#pragma once
#include <openxr/openxr.h>

class OpenXRApp {

  XrInstance instance = {0};
  XrSystemId systemId = {0};
  XrSession session = {0};

public:
  OpenXRApp();
  ~OpenXRApp();
  void createInstance(struct android_app *app);
  bool confirmGLES(int major, int minor);
  void createSession();
};
