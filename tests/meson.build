test_include_dirs = [config_h_dir, include_directories('../src')]

test_encoding = executable('test-file-encoding', 'test-file-encoding.cpp', notepad_lib_sources,
  include_directories: test_include_dirs,
  dependencies: [glibmm_dep, giomm_dep],
)
test('file-encoding', test_encoding)

test_recent_files = executable('test-recent-files', 'test-recent-files.cpp', notepad_lib_sources,
  include_directories: test_include_dirs,
  dependencies: [glibmm_dep, giomm_dep],
)
test('recent-files', test_recent_files)

# Compiles po/pl.po into a real <LC_MESSAGES>/notepad.mo tree in the build
# dir (mirroring the installed layout, which meson only assembles at
# `ninja install` time -- too late for `meson test`) so test-i18n.cpp can
# bindtextdomain() against actual catalog content, not a mock. Done at
# configure time with a single shell command rather than custom_target(),
# whose output-path handling for a nested LC_MESSAGES tree isn't worth the
# extra indirection for a test-only fixture.
msgfmt = find_program('msgfmt')
test_locale_dir = meson.current_build_dir() / 'test-locale'
test_pl_mo_dir = test_locale_dir / 'pl' / 'LC_MESSAGES'
run_command('sh', '-c',
  'mkdir -p "$1" && exec "$2" "$3" -o "$1/notepad.mo"',
  'sh', test_pl_mo_dir, msgfmt.full_path(),
  meson.current_source_dir() / '../po/pl.po',
  check: true,
)

test_i18n = executable('test-i18n', 'test-i18n.cpp', notepad_lib_sources,
  include_directories: test_include_dirs,
  dependencies: [glibmm_dep, giomm_dep],
)
test('i18n', test_i18n,
  env: {'NOTEPAD_TEST_LOCALEDIR': test_locale_dir},
)

test_bg_geometry = executable('test-bg-geometry', 'test-bg-geometry.cpp', notepad_lib_sources,
  include_directories: test_include_dirs,
  dependencies: [glibmm_dep, giomm_dep],
)
test('bg-geometry', test_bg_geometry)
