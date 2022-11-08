set(TARGET_NAME native_gles3)
add_library(${TARGET_NAME} SHARED android_main.cpp EGLState.cpp Renderer.cpp
                                  OpenXRApp.cpp)
target_link_libraries(${TARGET_NAME} PRIVATE native_app_glue android log EGL
                                             GLESv3 openxr_loader)
target_compile_definitions(${TARGET_NAME} PRIVATE XR_USE_PLATFORM_ANDROID
                                                  XR_USE_GRAPHICS_API_OPENGL_ES)
