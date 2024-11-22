# libs/mbtfile/meson.build

project('mbtfile', 'c')

mbtfile_sources = [
    'src/file_handler.c',
    'src/piece.c',
]

mbtfile_inc = include_directories('include')

mbtfile_dep = [
    dependency('openssl'),
    dependency('mbtutils'),
    # Ajoutez d'autres dépendances si nécessaire
]

mbtfile_lib = library(
    'mbtfile',
    mbtfile_sources,
    include_directories : [mbtfile_inc],
    dependencies : mbtfile_dep,
    install : true,
)

mbtfile_dep_decl = declare_dependency(
    include_directories : mbtfile_inc,
    link_with : mbtfile_lib,
    dependencies : mbtfile_dep,
)

# Configuration des flags de compilation pour les sanitizers
sanitizer_flags = [
    '-fsanitize=address',
    '-fsanitize=undefined',
    '-fno-omit-frame-pointer',
]

# Ajouter les tests unitaires avec Criterion
test_sources = [
    'tests/test_mbtfile.c',
]

criterion_dep = dependency('criterion', required : true)

test_mbtfile = executable(
    'test_mbtfile',
    test_sources,
    include_directories : [mbtfile_inc],
    dependencies : [mbtfile_dep_decl, criterion_dep],
    c_args : sanitizer_flags,
    link_args : sanitizer_flags,
)

test('Test mbtfile', test_mbtfile)

