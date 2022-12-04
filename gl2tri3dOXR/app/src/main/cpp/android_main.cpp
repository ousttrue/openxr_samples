#include "XrApp.h"
#include "openxr/openxr.h"
#include "render_scene.h"
#include "util_log.h"

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

  // create backbuffer
  for (int i = 0;; ++i) {
    auto view = xr.GetView(i);
    if (!view) {
      break;
    }

    view->createBackbuffers();
    //   LOGI("SwapchainImage[%d/%d] FBO:%d, TEXC:%d, TEXZ:%d, WH(%d, %d)", i,
    //        imgCnt, rtarget->fbo_id, rtarget->texc_id, rtarget->texz_id,
    //        sfc.width, sfc.height);
  }

  while (app->destroyRequested == 0) {
    // Read all pending events.
    for (;;) {
      int events;
      struct android_poll_source *source;

      int timeout = -1; // blocking
      if (appState.Resumed || xr.IsSessionRunning() || app->destroyRequested)
        timeout = 0; // non blocking

      if (ALooper_pollAll(timeout, nullptr, &events, (void **)&source) < 0) {
        break;
      }

      if (source != nullptr) {
        source->process(app, source);
      }
    }

    if (xr.UpdateFrame()) {
      XrPosef stagePose;
      if (xr.BeginFrame(&stagePose)) {
        for (int i = 0;; ++i) {
          auto view = xr.GetView(i);
          if (!view) {
            break;
          }
          auto renderTarget = view->acquireSwapchain();

          int x = view->projLayerView.subImage.imageRect.offset.x;
          int y = view->projLayerView.subImage.imageRect.offset.y;
          int w = view->projLayerView.subImage.imageRect.extent.width;
          int h = view->projLayerView.subImage.imageRect.extent.height;
          renderer.render_gles_scene(stagePose, renderTarget->fbo_id, x, y, w, h,
                                     view->projLayerView.fov,
                                     view->projLayerView.pose);
          view->releaseSwapchain();
        }
        xr.EndFrame();
      }
    }
  }
}
