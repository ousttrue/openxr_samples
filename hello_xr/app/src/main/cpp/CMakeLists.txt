# Copyright (c) 2017 The Khronos Group Inc.
#
# SPDX-License-Identifier: Apache-2.0
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
# Author:
#
set(TARGET_NAME hello_xr)
add_library(
  ${TARGET_NAME} MODULE
  main.cpp
  openxr_program.cpp
  logger.cpp
  platformplugin_factory.cpp
  platformplugin_android.cpp
  graphicsplugin_factory.cpp
  graphicsplugin_opengles.cpp
  common/gfxwrapper_opengl.c
  vulkan_shaders/frag.glsl
  vulkan_shaders/vert.glsl)

compile_glsl(run_hello_xr_glsl_compiles ${VULKAN_SHADERS})

target_include_directories(${TARGET_NAME} PRIVATE include common)

add_compile_definitions(${TARGET_NAME} PRIVATE XR_USE_PLATFORM_ANDROID
                        XR_USE_GRAPHICS_API_OPENGL_ES)
if(GLSLANG_VALIDATOR AND NOT GLSLC_COMMAND)
  target_compile_definitions(${TARGET_NAME} PRIVATE USE_GLSLANGVALIDATOR)
endif()

target_link_libraries(
  ${TARGET_NAME}
  android
  log
  android_native_app_glue
  openxr_loader
  GLESv3
  EGL)
target_link_options(${TARGET_NAME} PUBLIC -u ANativeActivity_onCreate)
