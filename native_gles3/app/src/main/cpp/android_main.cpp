#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state) {

    // loop waiting for stuff to do.

    auto sensorManager = ASensorManager_getInstance();
    auto accelerometerSensor =
                    ASensorManager_getDefaultSensor(sensorManager,
                        ASENSOR_TYPE_ACCELEROMETER);
    auto sensorEventQueue =
                    ASensorManager_createEventQueue(sensorManager,
                        state->looper, LOOPER_ID_USER, NULL, NULL);
    while (true) {
        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;
        bool animating = false;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((ident=ALooper_pollAll(animating ? 0 : -1, nullptr, &events,
                                      (void**)&source)) >= 0) {

            // Process this event.
            if (source != nullptr) {
                source->process(state, source);
            }

            // If a sensor has data, process it now.
            if (ident == LOOPER_ID_USER) {
                if (accelerometerSensor != nullptr) {
                    ASensorEvent event;
                    while (ASensorEventQueue_getEvents(sensorEventQueue,
                                                       &event, 1) > 0) {
                        LOGI("accelerometer: x=%f y=%f z=%f",
                             event.acceleration.x, event.acceleration.y,
                             event.acceleration.z);
                    }
                }
            }

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                return;
            }
        }

        if (animating) {
            // Done with events; draw next animation frame.

            // Drawing is throttled to the screen update rate, so there
            // is no need to do timing here.
        }
    }
}
