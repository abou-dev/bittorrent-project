mbtfile_sources = [
    'src/file_handler.c',
    'src/piece.c',
]

mbtfile_inc = include_directories('include')

mbtfile_dep = [
    dependency('openssl'),
]

mbtfile_lib = shared_library(
    'mbtfile',
    mbtfile_sources,
    include_directories : mbtfile_inc,
    dependencies : mbtfile_dep,
    install : true,
    install_dir : meson.current_build_dir(),
)

mbtfile_dep_decl = declare_dependency(
    include_directories : mbtfile_inc,
    link_with : mbtfile_lib,
    dependencies : mbtfile_dep,
)
