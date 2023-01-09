openxr_loader_dep = dependency('openxr_loader')
openxr_common_dep = dependency('openxr_common')
gl_dep = dependency('opengl')
executable('hello_xr', [
    'main.cpp',
    'openxr_program.cpp',
    'logger.cpp',
    'platformplugin_factory.cpp',
    'platformplugin_win32.cpp',
    'graphicsplugin_factory.cpp',
    # 'graphicsplugin_vulkan.cpp',
    'graphicsplugin_opengl.cpp',
    'common/gfxwrapper_opengl.c',
    # 'vulkan_shaders/frag.glsl',
    # 'vulkan_shaders/vert.glsl',
],
    install: true,
    cpp_args: ['-DXR_USE_PLATFORM_WIN32', '-DXR_USE_GRAPHICS_API_OPENGL'],
    dependencies: [openxr_loader_dep, openxr_common_dep, gl_dep],
)
