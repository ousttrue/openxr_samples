#pragma once

class OpenXRApp {
  class OpenXRAppImpl *impl_ = nullptr;

public:
  OpenXRApp(struct android_app *app);
  ~OpenXRApp();
};
