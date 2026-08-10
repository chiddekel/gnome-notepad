notepad_sources = [
  'main.cpp',
  'application.cpp',
  'window.cpp',
  'core/enums.cpp',
  'core/app-state.cpp',
  'core/i18n.cpp',
  'core/bg-geometry.cpp',
  'modules/settings.cpp',
  'modules/file.cpp',
  'modules/editor.cpp',
  'modules/commands.cpp',
  'modules/dialog.cpp',
  'modules/theme.cpp',
  'modules/background.cpp',
]

# Non-widget modules, reused by tests/meson.build so unit tests don't need
# to link the executable's main()/GTK entry point. Wrapped in files() so the
# paths stay anchored to src/ even when the variable is used from tests/.
notepad_lib_sources = files(
  'core/enums.cpp',
  'core/app-state.cpp',
  'core/i18n.cpp',
  'core/bg-geometry.cpp',
  'modules/file.cpp',
)

notepad_deps = [
  gtkmm_dep,
  glibmm_dep,
  giomm_dep,
  adwaita_dep,
]

resources = gnome.compile_resources(
  'notepad-resources',
  'resources/notepad.gresource.xml',
  source_dir: 'resources',
  c_name: 'notepad',
)

notepad_include_dirs = [config_h_dir, include_directories('.')]

executable('notepad', notepad_sources, resources,
  include_directories: notepad_include_dirs,
  dependencies: notepad_deps,
  install: true,
)
