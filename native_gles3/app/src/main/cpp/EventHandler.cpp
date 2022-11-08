#include "EventHandler.h"
#include "Renderer.h"
#include "android_logger.h"
#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <android_native_app_glue.h>
#include <assert.h>
#include <memory>

EventHandler::EventHandler() {}

EventHandler::~EventHandler() {}

void EventHandler::OnAppCmd(android_app *state, int32_t cmd) {
  auto self = reinterpret_cast<EventHandler *>(state->userData);
  self->onAppCmd(state, cmd);
}

void EventHandler::bind(android_app *state) {
  state->userData = this;
  state->onAppCmd = OnAppCmd;
}

void EventHandler::onAppCmd(android_app *state, int32_t cmd) {
  switch (cmd) {
  case APP_CMD_INIT_WINDOW:
    if (!renderer_) {
      initEGL3(state);
      eglQuerySurface(display_, surface_, EGL_WIDTH, &width_);
      eglQuerySurface(display_, surface_, EGL_HEIGHT, &height_);
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

bool EventHandler::initEGL3(android_app *state) {

  display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  eglInitialize(display_, nullptr, nullptr);

  EGLint numConfigs;
  /*
   * Here specify the attributes of the desired configuration.
   * Below, we select an EGLConfig with at least 8 bits per color
   * component compatible with on-screen windows
   */
  const EGLint attribs[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, EGL_NONE};
  EGLConfig config;
  eglChooseConfig(display_, attribs, &config, 1, &numConfigs);

  EGLint format;
  /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
   * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
   * As soon as we picked a EGLConfig, we can safely reconfigure the
   * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
  eglGetConfigAttrib(display_, config, EGL_NATIVE_VISUAL_ID, &format);
  surface_ = eglCreateWindowSurface(display_, config, state->window, nullptr);

  EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
  context_ = eglCreateContext(display_, config, EGL_NO_CONTEXT, contextAttribs);
  if (context_ == EGL_NO_CONTEXT) {
    LOGE("eglCreateContext failed with error 0x%04x", eglGetError());
    return false;
  }

  if (eglMakeCurrent(display_, surface_, surface_, context_) == EGL_FALSE) {
    LOGW("Unable to eglMakeCurrent");
    return false;
  }

  return true;
}
