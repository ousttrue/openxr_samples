#include "EventHandler.h"
#include "android_logger.h"
#include <android/sensor.h>
#include <android_native_app_glue.h>

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app *state) {

  // OpenXRApp app(state);

  EventHandler e;
  e.bind(state);

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
