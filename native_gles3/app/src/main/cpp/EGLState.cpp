#include "EGLState.h"
#include "android_logger.h"
#include <android_native_app_glue.h>

bool EGLState::initialize(android_app *state) {

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
