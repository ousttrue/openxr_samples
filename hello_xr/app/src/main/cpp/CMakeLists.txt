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

file(GLOB VULKAN_SHADERS "vulkan_shaders/*.glsl")

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
  ${VULKAN_SHADERS})
# set_target_properties(${TARGET_NAME} PROPERTIES FOLDER ${SAMPLES_FOLDER})

compile_glsl(run_hello_xr_glsl_compiles ${VULKAN_SHADERS})
add_dependencies(${TARGET_NAME} generate_openxr_header run_hello_xr_glsl_compiles)

target_include_directories(
  ${TARGET_NAME}
  PRIVATE # For including compiled shaders
          ${CMAKE_CURRENT_BINARY_DIR}
  # ${PROJECT_SOURCE_DIR}/src ${PROJECT_SOURCE_DIR}/src/common # for OpenXR
  # headers ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include # for
  # helper headers ${PROJECT_SOURCE_DIR}/external/include
)

add_compile_definitions(${TARGET_NAME} PRIVATE XR_USE_PLATFORM_ANDROID)
if(GLSLANG_VALIDATOR AND NOT GLSLC_COMMAND)
  target_compile_definitions(${TARGET_NAME} PRIVATE USE_GLSLANGVALIDATOR)
endif()

if(TARGET openxr-gfxwrapper)
  target_link_libraries(${TARGET_NAME} openxr-gfxwrapper)
endif()
target_link_libraries(${TARGET_NAME} openxr_loader ${ANDROID_LIBRARY}
                      ${ANDROID_LOG_LIBRARY} android_native_app_glue)
