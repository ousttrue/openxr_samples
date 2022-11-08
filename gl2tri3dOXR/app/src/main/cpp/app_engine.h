#pragma once

#include "util_egl.h"
#include "util_oxr.h"

class AppEngine {
public:
  explicit AppEngine(android_app *app);
  ~AppEngine();

  // Interfaces to android application framework
  struct android_app *AndroidApp(void) const;

  void InitOpenXR_GLES();
  void UpdateFrame();

private:
  struct android_app *m_app;

  XrInstance m_instance;
  XrSession m_session;
  XrSpace m_appSpace;
  XrSpace m_stageSpace;
  XrSystemId m_systemId;
  std::vector<viewsurface_t> m_viewSurface;

  int BeginFrame(XrTime *dpy_time,
                 std::vector<XrCompositionLayerProjectionView> &projLayerViews,
                 std::vector<XrView> &views);
  void RenderLayer(XrTime dpy_time, XrSpaceLocation &stageLoc, int i,
                   XrCompositionLayerProjectionView &layerView, XrView &view);
  void EndFrame(XrTime dpy_time,
                std::vector<XrCompositionLayerProjectionView> &projLayerViews);

public:
};

AppEngine *GetAppEngine(void);
