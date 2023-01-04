find_package(Vulkan REQUIRED)

set(VULKAN_SHADERS vulkan_shaders/frag.glsl vulkan_shaders/vert.glsl)
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

compile_glsl(run_hello_xr_glsl_compiles ${VULKAN_SHADERS})

set(TARGET_NAME hello_xr)
if(${ANDROID})
  add_library(
    ${TARGET_NAME} MODULE
    main.cpp
    openxr_program.cpp
    logger.cpp
    platformplugin_factory.cpp
    platformplugin_android.cpp
    graphicsplugin_factory.cpp
    graphicsplugin_vulkan.cpp
    vulkan_shaders/frag.glsl
    vulkan_shaders/vert.glsl)

  add_compile_definitions(${TARGET_NAME} PRIVATE XR_USE_PLATFORM_ANDROID
                          XR_USE_GRAPHICS_API_VULKAN)
  target_link_libraries(
    ${TARGET_NAME}
    android
    log
    android_native_app_glue
    openxr_loader
    # GLESv3
    # EGL
    Vulkan::Vulkan
    )
  target_link_options(${TARGET_NAME} PUBLIC -u ANativeActivity_onCreate)
else()
  add_executable(
    ${TARGET_NAME}
    main.cpp
    openxr_program.cpp
    logger.cpp
    platformplugin_factory.cpp
    platformplugin_win32.cpp
    graphicsplugin_factory.cpp
    graphicsplugin_vulkan.cpp
    vulkan_shaders/frag.glsl
    vulkan_shaders/vert.glsl)

  #
  # openxr_loader
  #
  get_filename_component(
    OPENXR_SDK_DIR
    ${CMAKE_CURRENT_LIST_DIR}/../../../../../third_party/OpenXR-SDK ABSOLUTE)

  # include(${CMAKE_CURRENT_LIST_DIR}/../../third_party/openxr.cmake)
  add_library(openxr_loader SHARED IMPORTED)
  set_target_properties(
    openxr_loader
    PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${OPENXR_SDK_DIR}/include
               IMPORTED_LOCATION
               ${OPENXR_SDK_DIR}/native/x64/release/bin/openxr_loader.dll
               IMPORTED_IMPLIB
               ${OPENXR_SDK_DIR}/native/x64/release/lib/openxr_loader.lib)

  add_compile_definitions(${TARGET_NAME} PRIVATE XR_USE_PLATFORM_WIN32
                          XR_USE_GRAPHICS_API_VULKAN)
  target_link_libraries(${TARGET_NAME} openxr_loader Vulkan::Vulkan)
endif()

target_include_directories(${TARGET_NAME} PRIVATE include vulkan_shaders
                                                  ${CMAKE_CURRENT_LIST_DIR})

if(GLSLANG_VALIDATOR AND NOT GLSLC_COMMAND)
  target_compile_definitions(${TARGET_NAME} PRIVATE USE_GLSLANGVALIDATOR)
endif()
