project(
    'bittorrent-client',
    'c',
    meson_version: '>=1.0.0',
    default_options: [
        'c_std=c99',
        'warning_level=2',
        'werror=true',
        'debug=true',
    ]
)

add_project_arguments('-D_GNU_SOURCE', language: 'c')

subdir('libs/mbtutils')
meson.override_dependency('mbtutils', mbtutils_dep)

subdir('libs/mbtbe')
meson.override_dependency('mbtbe', mbtbe_dep)

subdir('libs/mbtfile')
meson.override_dependency('mbtfile', mbtfile_dep_decl)
mbtutils_dep = dependency('mbtutils')
mbtbe_dep = dependency('mbtbe')
mbtfile_dep = dependency('mbtfile')

src_sources = files(
    'src/main.c',
)

include_dirs = [
    include_directories('libs/mbtutils/include'),
    include_directories('libs/mbtbe/include'),
    include_directories('libs/mbtfile/include'),
]

executable(
    'mbt',
    src_sources,
    dependencies: [
        mbtbe_dep,
        mbtutils_dep,
        mbtfile_dep,
    ],
    include_directories: include_dirs,
    install: true,
)

