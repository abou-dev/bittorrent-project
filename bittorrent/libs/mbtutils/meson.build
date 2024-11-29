mbtutils_sources = [
    'src/mbtstr.c',
]

mbtutils_inc = include_directories('include')

mbtutils_lib = shared_library(
    'mbtutils',
    mbtutils_sources,
    include_directories: mbtutils_inc,
    install: true,
)

mbtutils_dep = declare_dependency(
    include_directories: mbtutils_inc,
    link_with: mbtutils_lib,
)
alias_target('mbtutils', mbtutils_lib)
