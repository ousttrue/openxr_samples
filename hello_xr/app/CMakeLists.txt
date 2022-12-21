cmake_minimum_required(VERSION 3.0)
project(OPENXR)
# Entire project uses C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
unset(WIN32)
set(ANDROID TRUE)
get_filename_component(
  OPENXR_SDK_DIR ${CMAKE_CURRENT_LIST_DIR}/../../third_party/OpenXR-SDK
  ABSOLUTE)
get_filename_component(
  MOBILE_SDK_DIR ${CMAKE_CURRENT_LIST_DIR}/../../third_party/OpenXRMobileSDK
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
# include(${CMAKE_CURRENT_LIST_DIR}/../../third_party/openxr.cmake)
add_library(openxr_loader SHARED IMPORTED)
set_target_properties(
  openxr_loader
  PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${OPENXR_SDK_DIR}/include
    IMPORTED_LOCATION
    ${MOBILE_SDK_DIR}/OpenXR/Libs/Android/${ANDROID_ABI}/Release/libopenxr_loader.so
)
# subdirs(${OPENXR_SDK_DIR})

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
