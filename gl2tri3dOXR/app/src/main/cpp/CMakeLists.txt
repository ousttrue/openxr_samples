#
# Copyright (C) The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#
cmake_minimum_required(VERSION 3.4.1)
project(gl2tri3dOXR VERSION 0.1.0)

set(PROJTOP ${CMAKE_CURRENT_SOURCE_DIR}/../../../../../)

set(OXR_KHRONOS_DIR ${PROJTOP}/third_party/OpenXR-SDK)
set(OXR_LOADER_DIR ${PROJTOP}/third_party/OpenXRMobileSDK)

# ------------------------------------------------------------------------
# native_app_glue
# ------------------------------------------------------------------------
set(${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")
add_library(
  native_app_glue STATIC
  ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)
target_include_directories(
  native_app_glue PUBLIC ${ANDROID_NDK}/sources/android/native_app_glue)

# now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall -Werror")

# Export ANativeActivity_onCreate(), Refer to:
# https://github.com/android-ndk/ndk/issues/381.
set(CMAKE_SHARED_LINKER_FLAGS
    "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

# ------------------------------------------------------------------------
# openxr_loader
# ------------------------------------------------------------------------
add_library(openxr_loader SHARED IMPORTED)
set_property(
  TARGET openxr_loader
  PROPERTY
    IMPORTED_LOCATION
    ${OXR_LOADER_DIR}/OpenXR/Libs/Android/${ANDROID_ABI}/${CMAKE_BUILD_TYPE}/libopenxr_loader.so
)

# ------------------------------------------------------------------------
# openxr_app
# ------------------------------------------------------------------------
add_library(
  openxr_app SHARED
  android_main.cpp
  XrApp.cpp
  render_scene.cpp
  common/util_egl.c
  common/util_shader.c
  common/util_matrix.c
  common/util_debugstr.c
  common/util_render_target.cpp
  common/assertegl.c
  common/assertgl.c
  common/winsys/winsys_null.c)

target_include_directories(
  openxr_app
  PRIVATE ${ANDROID_NDK}/sources/android/native_app_glue
          ${OXR_KHRONOS_DIR}/include/ ${OXR_KHRONOS_DIR}/src/ common/
          common/winsys/ ${PROJTOP}/third_party)

add_definitions(-DXR_OS_ANDROID)
add_definitions(-DXR_USE_PLATFORM_ANDROID)
add_definitions(-DXR_USE_GRAPHICS_API_OPENGL_ES)

# add lib dependencies
target_link_libraries(
  openxr_app
  android
  native_app_glue
  openxr_loader
  EGL
  GLESv3
  log)
