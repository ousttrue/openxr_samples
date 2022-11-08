#include "EGLState.h"
#include "OpenXRApp.h"
#include "Renderer.h"
#include "android_logger.h"
#include <android_native_app_glue.h>
#include <assert.h>

static void onAppCmd(android_app *state, int32_t cmd) {
  switch (cmd) {
  case APP_CMD_INIT_WINDOW:
    break;

  case APP_CMD_TERM_WINDOW:
    break;
  }
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app *state) {

  // state->userData = this;
  state->onAppCmd = onAppCmd;

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

  // space
  // view

  while (true) {
    while (true) {
      int events;
      android_poll_source *source;
      auto ident = ALooper_pollAll(0, nullptr, &events, (void **)&source);
      {
        if (ident < 0) {
          break;
        }
      }
      if (source) {
        source->process(state, source);
      }
      if (state->destroyRequested != 0) {
        return;
      }
    }
  }
}
