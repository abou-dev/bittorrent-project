project(
    'mbtbe',
    'c',
    meson_version : '>=1.0.0',
    default_options : [
        'c_std=c99',
        'warning_level=2',
        'werror=true',
        'debug=true',
        'c_args=-Wno-deprecated-declarations'
    ]
)

openssl_dep = dependency('openssl', required : true)

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
    dependencies : [openssl_dep], # Link with OpenSSL
    install : true,            
    install_dir : meson.current_build_dir() 
)

mbtbe_dep = declare_dependency(
    link_with : mbtbe,
    include_directories : inc,
    dependencies : [openssl_dep] # Export OpenSSL dependency
)
