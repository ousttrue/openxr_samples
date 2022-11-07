cmake_minimum_required(VERSION 3.22.1)
project(native_gles3 VERSION 0.1.0)

#
# native_app_glue as a static lib
#
set(TARGET_NAME native_app_glue)
set(ANDROID_NATIVE_APP_GLUE
    ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue)
add_library(${TARGET_NAME} STATIC
            ${ANDROID_NATIVE_APP_GLUE}/android_native_app_glue.c)
target_include_directories(${TARGET_NAME} PUBLIC ${ANDROID_NATIVE_APP_GLUE})

# Export ANativeActivity_onCreate(), Refer to:
# https://github.com/android-ndk/ndk/issues/381.
target_link_options(${TARGET_NAME} PUBLIC -u ANativeActivity_onCreate)

subdirs(src/main/cpp)
