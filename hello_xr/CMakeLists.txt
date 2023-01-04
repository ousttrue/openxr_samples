cmake_minimum_required(VERSION 3.0)
project(OPENXR)
# Entire project uses C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
unset(WIN32)
set(ANDROID TRUE)
get_filename_component(
  OPENXR_SDK_DIR ../third_party/OpenXR-SDK
  ABSOLUTE)
get_filename_component(
  MOBILE_SDK_DIR ../third_party/OpenXRMobileSDK
  ABSOLUTE)

#
# native_app_glue as a static lib
#
set(TARGET_NAME android_native_app_glue)
set(ANDROID_NATIVE_APP_GLUE
    ${CMAKE_ANDROID_NDK}/sources/android/native_app_glue)
add_library(${TARGET_NAME} STATIC
            ${ANDROID_NATIVE_APP_GLUE}/android_native_app_glue.c)
target_include_directories(${TARGET_NAME} PUBLIC ${ANDROID_NATIVE_APP_GLUE})
target_compile_options(${TARGET_NAME} PRIVATE -Wno-unused-parameter)

#
# openxr_loader
#
add_library(openxr_loader SHARED IMPORTED)
set_target_properties(
  openxr_loader
  PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${OPENXR_SDK_DIR}/include
    IMPORTED_LOCATION
    ${MOBILE_SDK_DIR}/OpenXR/Libs/Android/${ANDROID_ABI}/Release/libopenxr_loader.so
)
# subdirs(${OPENXR_SDK_DIR})

subdirs(cpp)
