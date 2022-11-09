#pragma once
#include "openxr/openxr.h"
#include "util_egl.h"
#include "util_oxr.h"
#include "util_render_target.h"
#include <functional>
#include <memory>
#include <stdint.h>

using RenderFunc =
    std::function<int(XrCompositionLayerProjectionView &layerView,
                      render_target &rtarget, const XrPosef &stagePose)>;

struct viewsurface {
  XrSwapchain swapchain;
  uint32_t width;
  uint32_t height;

  XrView view;
  XrCompositionLayerProjectionView projLayerView;

  std::vector<std::shared_ptr<render_target>> backbuffers;

  std::vector<XrSwapchainImageOpenGLESKHR> getSwapchainImages() const;
  void createBackbuffers();

  std::shared_ptr<render_target> acquireSwapchain();
  void releaseSwapchain() const;
};

class AppEngine {
  struct android_app *m_app;

  XrInstance m_instance;
  XrSession m_session;
  XrSpace m_appSpace;
  XrSpace m_stageSpace;
  XrSystemId m_systemId;
  std::vector<viewsurface> m_viewSurface;

  XrTime m_displayTime;

public:
  explicit AppEngine(android_app *app);
  ~AppEngine();

  // Interfaces to android application framework
  struct android_app *AndroidApp(void) const;

  void InitOpenXR_GLES();
  void CreateSession();
  bool UpdateFrame();
  bool BeginFrame(XrPosef *stagePose);
  viewsurface *GetView(size_t i);
  void EndFrame();
  uint32_t SwapchainIndex() const;
};

AppEngine *GetAppEngine(void);
