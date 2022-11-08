#include "Engine.h"
#include "Renderer.h"
#include "android_logger.h"
#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <android_native_app_glue.h>
#include <assert.h>
#include <memory>

class EngineImpl {
  EGLDisplay display_ = 0;
  EGLSurface surface_ = 0;
  EGLint width_ = 0;
  EGLint height_ = 0;
  Renderer *renderer_ = nullptr;

  bool init(android_app *state) {

    display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display_, nullptr, nullptr);

    EGLint numConfigs;
    /*
     * Here specify the attributes of the desired configuration.
     * Below, we select an EGLConfig with at least 8 bits per color
     * component compatible with on-screen windows
     */
    const EGLint attribs[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                              EGL_BLUE_SIZE,    8,
                              EGL_GREEN_SIZE,   8,
                              EGL_RED_SIZE,     8,
                              EGL_NONE};
    /* Here, the application chooses the configuration it desires.
     * find the best match if possible, otherwise use the very first one
     */
    eglChooseConfig(display_, attribs, nullptr, 0, &numConfigs);

    std::unique_ptr<EGLConfig[]> supportedConfigs(new EGLConfig[numConfigs]);
    assert(supportedConfigs);

    eglChooseConfig(display_, attribs, supportedConfigs.get(), numConfigs,
                    &numConfigs);
    assert(numConfigs);

    auto i = 0;
    EGLConfig config = nullptr;
    for (; i < numConfigs; i++) {
      auto &cfg = supportedConfigs[i];
      EGLint r, g, b, d;
      if (eglGetConfigAttrib(display_, cfg, EGL_RED_SIZE, &r) &&
          eglGetConfigAttrib(display_, cfg, EGL_GREEN_SIZE, &g) &&
          eglGetConfigAttrib(display_, cfg, EGL_BLUE_SIZE, &b) &&
          eglGetConfigAttrib(display_, cfg, EGL_DEPTH_SIZE, &d) && r == 8 &&
          g == 8 && b == 8 && d == 0) {

        config = supportedConfigs[i];
        break;
      }
    }
    if (i == numConfigs) {
      config = supportedConfigs[0];
    }

    if (config == nullptr) {
      LOGW("Unable to initialize EGLConfig");
      return false;
    }

    EGLint format;
    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    eglGetConfigAttrib(display_, config, EGL_NATIVE_VISUAL_ID, &format);
    surface_ = eglCreateWindowSurface(display_, config, state->window, nullptr);
    auto context = eglCreateContext(display_, config, nullptr, nullptr);

    if (eglMakeCurrent(display_, surface_, surface_, context) == EGL_FALSE) {
      LOGW("Unable to eglMakeCurrent");
      return -1;
    }

    eglQuerySurface(display_, surface_, EGL_WIDTH, &width_);
    eglQuerySurface(display_, surface_, EGL_HEIGHT, &height_);

    return true;
  }

public:
  void OnAppCmd(struct android_app *app, int32_t cmd) {
    switch (cmd) {
    case APP_CMD_INIT_WINDOW:
      if (!renderer_) {
        init(app);
        renderer_ = new Renderer();
      }
      renderer_->draw(width_, height_);
      eglSwapBuffers(this->display_, this->surface_);
      break;

    case APP_CMD_TERM_WINDOW:
      delete renderer_;
      renderer_ = nullptr;
      break;
    }
  }
};

engine::engine() : impl_(new EngineImpl) {}
engine::~engine() { delete impl_; }

void OnAppCmd(struct android_app *app, int32_t cmd) {
  auto e = static_cast<engine *>(app->userData);
  e->impl_->OnAppCmd(app, cmd);
}
