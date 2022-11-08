#pragma once

#include "openxr/openxr.h"
#include "util_egl.h"
#include "util_oxr.h"
#include <functional>
#include <stdint.h>

struct FrameInfo {
  bool isRunning;
  XrTime time;
  XrSpaceLocation stageLoc;
  uint32_t viewCount;
};

using RenderFunc =
    std::function<int(XrCompositionLayerProjectionView &layerView,
                      render_target_t &rtarget, const XrPosef &stagePose)>;
class AppEngine {
public:
  explicit AppEngine(android_app *app);
  ~AppEngine();

  // Interfaces to android application framework
  struct android_app *AndroidApp(void) const;

  void InitOpenXR_GLES();
  void CreateSession();
  bool UpdateFrame();
  void BeginFrame(FrameInfo *frame);

  void RenderLayer(const FrameInfo &frame, int i, const RenderFunc &func);
  void EndFrame(XrTime dpy_time);

private:
  struct android_app *m_app;

  XrInstance m_instance;
  XrSession m_session;
  XrSpace m_appSpace;
  XrSpace m_stageSpace;
  XrSystemId m_systemId;
  std::vector<viewsurface> m_viewSurface;

  std::vector<XrView> m_views;
  std::vector<XrCompositionLayerProjectionView> m_projLayerViews;

public:
};

AppEngine *GetAppEngine(void);
