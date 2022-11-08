#include "EGLState.h"
#include "OpenXRApp.h"
#include "Renderer.h"
#include "android_logger.h"
#include <android_native_app_glue.h>
#include <assert.h>

struct AndroidAppState {
  ANativeWindow *NativeWindow = nullptr;
  bool Resumed = false;
};

static void ProcessAndroidCmd(struct android_app *app, int32_t cmd) {
  AndroidAppState *appState = (AndroidAppState *)app->userData;

  switch (cmd) {
  case APP_CMD_START:
    LOGI("APP_CMD_START");
    break;

  case APP_CMD_RESUME:
    LOGI("APP_CMD_RESUME");
    appState->Resumed = true;
    break;

  case APP_CMD_PAUSE:
    LOGI("APP_CMD_PAUSE");
    appState->Resumed = false;
    break;

  case APP_CMD_STOP:
    LOGI("APP_CMD_STOP");
    break;

  case APP_CMD_DESTROY:
    LOGI("APP_CMD_DESTROY");
    appState->NativeWindow = NULL;
    break;

  // The window is being shown, get it ready.
  case APP_CMD_INIT_WINDOW:
    LOGI("APP_CMD_INIT_WINDOW");
    appState->NativeWindow = app->window;
    break;

  // The window is being hidden or closed, clean it up.
  case APP_CMD_TERM_WINDOW:
    LOGI("APP_CMD_TERM_WINDOW");
    appState->NativeWindow = NULL;
    break;
  }
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app *state) {

  AndroidAppState appState = {};
  state->userData = &appState;
  state->onAppCmd = ProcessAndroidCmd;

  // instance
  OpenXRApp oxr;
  oxr.createInstance(state);

  // graphics
  EGLState egl;
  if (!egl.initialize(state)) {
    LOGE("fail to initialize EGL");
    return;
  }

  Renderer renderer;
  int major = renderer.major();
  int minor = renderer.minor();
  if (!oxr.confirmGLES(major, minor)) {
    LOGE("fail to confirm GLES %d.%d", major, minor);
    return;
  }

  // session
  oxr.createSession();

  while (state->destroyRequested == 0) {
    // Read all pending events.
    while (true) {
      int events;
      struct android_poll_source *source;

      int timeout = -1; // blocking
      if (appState.Resumed || oxr.isSessionRunning() ||
          state->destroyRequested) {
        timeout = 0; // non blocking
      }

      if (ALooper_pollAll(timeout, nullptr, &events, (void **)&source) < 0) {
        break;
      }

      if (source != nullptr) {
        source->process(state, source);
      }
    }

    auto time = oxr.beginFrame();
    for (auto &view : oxr.viewSwapchains_) {
      // renderer.renderView(view.swapchain);
    }
    oxr.endFrame(time);
  }
}
