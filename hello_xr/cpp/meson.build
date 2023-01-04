openxr_loader_dep = dependency('openxr_loader')
openxr_common_dep = dependency('openxr_common')
executable('hello_xr', [
    'main.cpp',
    'openxr_program.cpp',
    'logger.cpp',
    'platformplugin_factory.cpp',
    'platformplugin_win32.cpp',
    'graphicsplugin_factory.cpp',
    'graphicsplugin_vulkan.cpp',
    # 'vulkan_shaders/frag.glsl',
    # 'vulkan_shaders/vert.glsl',
],
    install: true,
    dependencies: [openxr_loader_dep, openxr_common_dep],
)
