#pragma once
#include <functional>
#include <memory>
#include <openxr/openxr.h>
#include <stdint.h>
#include <vector>

using RenderFunc = std::function<int(
    const XrPosef &stagePose, uint32_t fbo, int x, int y, int w, int h,
    const XrFovf &fov, const XrPosef &viewPose)>;

class XrApp {

  XrInstance m_instance;
  XrSession m_session;
  XrSystemId m_systemId;

  XrSessionState m_session_state = XR_SESSION_STATE_UNKNOWN;
  bool m_session_running = false;

  XrSpace m_appSpace;
  XrSpace m_stageSpace;
  std::vector<std::shared_ptr<struct viewsurface>> m_viewSurface;

  XrTime m_displayTime;

  void oxr_check_errors(XrResult ret, const char *func, const char *fname,
                        int line);
  int oxr_handle_session_state_changed(XrSession session,
                                       XrEventDataSessionStateChanged &ev,
                                       bool *exitLoop, bool *reqRestart);
  int oxr_poll_events(XrInstance instance, XrSession session, bool *exit_loop,
                      bool *req_restart);

  bool BeginFrame(XrPosef *stagePose);
  // viewsurface *GetView(size_t i);
  void EndFrame();
  uint32_t SwapchainIndex() const;

public:
  bool IsSessionRunning() const { return m_session_running; }
  void CreateInstance(struct android_app *app);
  void CreateGraphics();
  void CreateSession();
  bool UpdateFrame();
  void Render(const RenderFunc &func);
};
