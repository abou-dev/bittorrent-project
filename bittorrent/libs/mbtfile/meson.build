mbtfile_sources = [
    'src/file_handler.c',
    'src/piece.c',
]

mbtfile_inc = include_directories('include')
openssl_dep = dependency('openssl', required: true)
mbtutils_dep = dependency('mbtutils', required: true)
mbtbe_dep = dependency('mbtbe', required: true)
mbtfile_deps = [
    openssl_dep,
    mbtutils_dep,
   mbtbe_dep,
]

mbtfile_lib = shared_library(
    'mbtfile',
    mbtfile_sources,
    include_directories: mbtfile_inc,
    dependencies: mbtfile_deps,
    install: true,
    install_dir: meson.current_build_dir(),
)

mbtfile_dep_decl = declare_dependency(
    include_directories: mbtfile_inc,
    link_with: mbtfile_lib,
    dependencies: mbtfile_deps,
)
