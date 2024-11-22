project(
    'mbtbe',
    'c',
    meson_version : '>=1.0.0',
    default_options : [
        'c_std=c99',
        'warning_level=2',
        'werror=true',
        'debug=true'
    ]
)

inc = include_directories('include')

mbtbe_sources = files(
    'src/torrent.c',
    'src/torrent_getters.c',
    'src/torrent_files_getters.c',
    'src/encode.c',
    'src/decode.c',
    'src/mbtstr.c',
    'src/node.c'
)

mbtbe = shared_library(
    'mbtbe',                   
    mbtbe_sources,             
    include_directories : inc, 
    install : true,            
    install_dir : meson.current_build_dir() 
)

mbtbe_dep = declare_dependency(
    link_with : mbtbe,
    include_directories : inc
)



	

