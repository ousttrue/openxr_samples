project('OpenXR-SDK-Source', 'c')
openxr_common_inc = include_directories('src', 'external/include')
openxr_common_lib = static_library('openxr_common', [
],
    include_directories: openxr_common_inc,
)
openxr_common_dep = declare_dependency(
    include_directories: openxr_common_inc,
)
