project('mbtutils', 'c')

mbtutils_sources = [
    'src/str.c',
    'src/view.c',
]

mbtutils_inc = include_directories('include')

mbtutils_dep = [
    # dependency('mbtbe'),
]

mbtutils_lib = library(
    'mbtutils',
    mbtutils_sources,
    include_directories : [mbtutils_inc],
    dependencies : mbtutils_dep,
    install : true,
)

mbtutils_dep_decl = declare_dependency(
    include_directories : [mbtutils_inc],
    link_with : mbtutils_lib,
    dependencies : mbtutils_dep,
)

sanitizer_flags = [
    '-fsanitize=address',
    '-fsanitize=undefined',
    '-fno-omit-frame-pointer',
]

test_sources = [
    'tests/test_mbtutils.c',
]

criterion_dep = dependency('criterion', required : true)

test_mbtutils = executable(
    'test_mbtutils',
    test_sources,
    include_directories : [mbtutils_inc],
    dependencies : [mbtutils_dep_decl, criterion_dep],
    c_args : sanitizer_flags,
    link_args : sanitizer_flags,
)

test('Test mbtutils', test_mbtutils)

