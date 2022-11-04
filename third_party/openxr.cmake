#
# openxr-gfxwrapper: This is a little helper library for setting up OpenGL
#
set(TARGET_NAME openxr-gfxwrapper)
add_library(
  ${TARGET_NAME} STATIC
  ${CMAKE_CURRENT_LIST_DIR}/OpenXR-SDK-Source/src/common/gfxwrapper_opengl.c)
target_include_directories(
  ${TARGET_NAME}
  PUBLIC ${CMAKE_CURRENT_LIST_DIR}/OpenXR-SDK-Source/external/include
         ${CMAKE_CURRENT_LIST_DIR}/OpenXR-SDK-Source/src)
target_link_libraries(${TARGET_NAME} PRIVATE android_native_app_glue)

#
# openxr_loader
#
find_package(PythonInterp 3)
set(LOADER_FOLDER "Loader")
set(CODEGEN_FOLDER "Generated")
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/OpenXR-SDK-Source/src/cmake)
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/OpenXR-SDK-Source)
include(StdFilesystemFlags)
find_package(Threads REQUIRED)
option(DYNAMIC_LOADER "Build the loader as a .dll library" ON)
add_definitions(-DXR_OS_ANDROID)
find_library(ANDROID_LIBRARY NAMES android)
find_library(ANDROID_LOG_LIBRARY NAMES log)

# General code generation macro used by several targets.
macro(run_xr_xml_generate dependency output)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${output}" AND NOT
                                                        BUILD_FORCE_GENERATION)
    # pre-generated found
    message(STATUS "Found and will use pre-generated ${output} in source tree")
    list(APPEND GENERATED_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/${output}")
  else()
    if(NOT PYTHON_EXECUTABLE)
      message(
        FATAL_ERROR
          "Python 3 not found, but pre-generated ${CMAKE_CURRENT_SOURCE_DIR}/${output} not found"
      )
    endif()
    add_custom_command(
      OUTPUT ${output}
      COMMAND
        ${CMAKE_COMMAND} -E env "PYTHONPATH=${CODEGEN_PYTHON_PATH}"
        ${PYTHON_EXECUTABLE} ${PROJECT_SOURCE_DIR}/src/scripts/src_genxr.py
        -registry ${PROJECT_SOURCE_DIR}/specification/registry/xr.xml ${output}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      DEPENDS "${PROJECT_SOURCE_DIR}/specification/registry/xr.xml"
              "${PROJECT_SOURCE_DIR}/specification/scripts/generator.py"
              "${PROJECT_SOURCE_DIR}/specification/scripts/reg.py"
              "${PROJECT_SOURCE_DIR}/src/scripts/${dependency}"
              "${PROJECT_SOURCE_DIR}/src/scripts/src_genxr.py"
              ${ARGN}
      COMMENT "Generating ${output} using ${PYTHON_EXECUTABLE} on ${dependency}"
    )
    set_source_files_properties(${output} PROPERTIES GENERATED TRUE)
    list(APPEND GENERATED_OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${output}")
    list(APPEND GENERATED_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${output}")
  endif()
endmacro()

# Custom target for generated dispatch table sources, used by several targets.
set(GENERATED_OUTPUT)
set(GENERATED_DEPENDS)
run_xr_xml_generate(utility_source_generator.py xr_generated_dispatch_table.h)
run_xr_xml_generate(utility_source_generator.py xr_generated_dispatch_table.c)
if(GENERATED_DEPENDS)
  add_custom_target(xr_global_generated_files DEPENDS ${GENERATED_DEPENDS})
else()
  add_custom_target(xr_global_generated_files)
endif()
set_target_properties(xr_global_generated_files PROPERTIES FOLDER
                                                           ${CODEGEN_FOLDER})

set(COMMON_GENERATED_OUTPUT ${GENERATED_OUTPUT})
set(COMMON_GENERATED_DEPENDS ${GENERATED_DEPENDS})

subdirs(${CMAKE_CURRENT_LIST_DIR}/OpenXR-SDK-Source/include
        ${CMAKE_CURRENT_LIST_DIR}/OpenXR-SDK-Source/src/loader)
