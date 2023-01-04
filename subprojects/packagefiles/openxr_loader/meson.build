project('openxr_loader', 'c')

cc = meson.get_compiler('c')
lib_dir = meson.current_source_dir() + '/native/x64/release/lib'
libs = []
libs += cc.find_library('openxr_loader', dirs: lib_dir)

openxr_loader_dep = declare_dependency(
    include_directories: include_directories('include'),
    dependencies: libs,
    compile_args: ['-DXR_USE_PLATFORM_WIN32'],
)

install_data('native/x64/release/bin/openxr_loader.dll', install_dir: 'bin')
