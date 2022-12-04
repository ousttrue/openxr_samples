#pragma once
#ifdef __ANDROID__
#include <android/native_window.h>
#include <android_native_app_glue.h>
#include <jni.h>
#include <sys/system_properties.h>
#endif

#include "util_egl.h"
#include <GLES3/gl31.h>

#include "util_render_target.h"
#include <functional>
#include <memory>
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_reflection.h>
#include <stdint.h>
#include <vector>

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

class XrApp {

  XrInstance m_instance;
  XrSession m_session;
  XrSystemId m_systemId;

  XrSessionState m_session_state = XR_SESSION_STATE_UNKNOWN;
  bool m_session_running = false;

  XrSpace m_appSpace;
  XrSpace m_stageSpace;
  std::vector<viewsurface> m_viewSurface;

  XrTime m_displayTime;

  void oxr_check_errors(XrResult ret, const char *func, const char *fname,
                        int line);
  int oxr_handle_session_state_changed(XrSession session,
                                       XrEventDataSessionStateChanged &ev,
                                       bool *exitLoop, bool *reqRestart);
  int oxr_poll_events(XrInstance instance, XrSession session, bool *exit_loop,
                      bool *req_restart);

public:
  bool IsSessionRunning() { return m_session_running; }
  void CreateInstance(struct android_app *app);
  void CreateGraphics();
  void CreateSession();
  bool UpdateFrame();
  bool BeginFrame(XrPosef *stagePose);
  viewsurface *GetView(size_t i);
  void EndFrame();
  uint32_t SwapchainIndex() const;
};

XrApp *GetAppEngine(void);
