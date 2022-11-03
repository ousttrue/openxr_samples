cmake_minimum_required(VERSION 3.0)
project(OPENXR)
set(OPENXR_SDK_DIR ${CMAKE_CURRENT_LIST_DIR}/../../third_party/OpenXR-SDK-Source)

# Entire project uses C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# This is a little helper library for setting up OpenGL
add_library(
  openxr-gfxwrapper STATIC ${OPENXR_SDK_DIR}/src/common/gfxwrapper_opengl.c
                           ${OPENXR_SDK_DIR}/src/common/gfxwrapper_opengl.h)
target_include_directories(openxr-gfxwrapper
                           PUBLIC ${PROJECT_SOURCE_DIR}/external/include)
if(OPENGL_FOUND)
  if(TARGET OpenGL::OpenGL)
    target_link_libraries(openxr-gfxwrapper PUBLIC OpenGL::OpenGL)
  elseif(TARGET OpenGL::GL)
    target_link_libraries(openxr-gfxwrapper PUBLIC OpenGL::GL)
  else()
    target_link_libraries(openxr-gfxwrapper PUBLIC ${OPENGL_LIBRARIES})
  endif()
endif()
if(OpenGLES_FOUND)
  if(TARGET OpenGLES::OpenGLESv3)
    target_link_libraries(openxr-gfxwrapper PUBLIC OpenGLES::OpenGLESv3)
  elseif(TARGET OpenGLES::OpenGLESv2)
    target_link_libraries(openxr-gfxwrapper PUBLIC OpenGLES::OpenGLESv2)
  else()
    message(FATAL_ERROR "Should not get here!")
  endif()
  target_link_libraries(openxr-gfxwrapper PUBLIC EGL::EGL)
endif()

target_include_directories(openxr-gfxwrapper PUBLIC ${ANDROID_NATIVE_APP_GLUE})

# Note: For some reason, just adding this to the gfxwrapper library results in
# failure at load time. So, each consuming target must add
# $<TARGET_OBJECTS:android_native_app_glue> to their sources
add_library(android_native_app_glue OBJECT
            "${ANDROID_NATIVE_APP_GLUE}/android_native_app_glue.c")
target_include_directories(android_native_app_glue
                           PUBLIC ${ANDROID_NATIVE_APP_GLUE})
target_compile_options(android_native_app_glue PRIVATE -Wno-unused-parameter)
message(
  STATUS
    "Enabling OpenGL support in hello_xr, loader_test, and conformance, if configured"
)

set(HELPER_FOLDER "Helpers")
function(compile_glsl run_target_name)
  set(glsl_output_files "")
  foreach(in_file IN LISTS ARGN)
    get_filename_component(glsl_stage ${in_file} NAME_WE)
    set(out_file ${CMAKE_CURRENT_BINARY_DIR}/${glsl_stage}.spv)
    if(GLSL_COMPILER)
      # Run glslc if we can find it
      add_custom_command(
        OUTPUT ${out_file}
        COMMAND ${GLSL_COMPILER} -mfmt=c -fshader-stage=${glsl_stage} ${in_file}
                -o ${out_file}
        DEPENDS ${in_file})
    elseif(GLSLANG_VALIDATOR)
      # Run glslangValidator if we can find it
      add_custom_command(
        OUTPUT ${out_file}
        COMMAND ${GLSLANG_VALIDATOR} -V -S ${glsl_stage} ${in_file} -x -o
                ${out_file}
        DEPENDS ${in_file}
        VERBATIM)
    else()
      # Use the precompiled .spv files
      get_filename_component(glsl_src_dir ${in_file} DIRECTORY)
      set(precompiled_file ${glsl_src_dir}/${glsl_stage}.spv)
      configure_file(${precompiled_file} ${out_file} COPYONLY)
    endif()
    list(APPEND glsl_output_files ${out_file})
  endforeach()
  add_custom_target(${run_target_name} ALL DEPENDS ${glsl_output_files})
  set_target_properties(${run_target_name} PROPERTIES FOLDER ${HELPER_FOLDER})

endfunction()

subdirs(src/main/cpp)
