#include "XrApp.h"
#include "render_scene.h"
#include "util_log.h"

#ifdef __ANDROID__
#include <android/native_window.h>
#include <android_native_app_glue.h>
#include <jni.h>
#include <sys/system_properties.h>
#endif

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

static void process_android_event(struct android_app *app,
                                  const AndroidAppState &appState,
                                  const XrApp &xr) {
  // Read all pending android events.
  for (;;) {

    int timeout = -1; // blocking
    if (appState.Resumed || xr.IsSessionRunning() || app->destroyRequested) {
      timeout = 0; // non blocking
    }

    int events;
    struct android_poll_source *source;
    if (ALooper_pollAll(timeout, nullptr, &events, (void **)&source) < 0) {
      break;
    }

    if (source != nullptr) {
      source->process(app, source);
    }
  }
}

/*--------------------------------------------------------------------------- *
 *      M A I N    F U N C T I O N
 *--------------------------------------------------------------------------- */
void android_main(struct android_app *app) {
  AndroidAppState appState = {};
  app->userData = &appState;
  app->onAppCmd = ProcessAndroidCmd;

  XrApp xr;
  xr.CreateInstance(app);
  xr.CreateGraphics();

  Renderer renderer;
  renderer.init_gles_scene();

  xr.CreateSession();

  while (app->destroyRequested == 0) {
    process_android_event(app, appState, xr);

    xr.Render(std::bind(&Renderer::render_gles_scene, &renderer,
                        std::placeholders::_1, std::placeholders::_2,
                        std::placeholders::_3, std::placeholders::_4,
                        std::placeholders::_5, std::placeholders::_6,
                        std::placeholders::_7, std::placeholders::_8));
  }
}
