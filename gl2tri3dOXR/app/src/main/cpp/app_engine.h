#pragma once

#include "openxr/openxr.h"
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
  std::vector<XrView> m_views;
  std::vector<XrCompositionLayerProjectionView> m_projLayerViews;

  XrTime BeginFrame();
  void RenderLayer(XrTime dpy_time, XrSpaceLocation &stageLoc, int i,
                   XrCompositionLayerProjectionView &layerView, XrView &view);
  void EndFrame(XrTime dpy_time);

public:
};

AppEngine *GetAppEngine(void);
