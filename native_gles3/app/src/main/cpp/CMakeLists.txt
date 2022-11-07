set(TARGET_NAME native_gles3)
add_library(${TARGET_NAME} SHARED android_main.cpp)
target_link_libraries(${TARGET_NAME} PRIVATE native_app_glue android log)
