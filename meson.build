project('notepad', 'cpp', 'c',
  version: '0.1.12-Beta',
  license: 'MIT',
  default_options: [
    'cpp_std=c++17',
    'warning_level=2',
    # gtkmm/glibmm/pangomm/cairomm/libsigc++ are vendored as vanilla wrap
    # subprojects inside this same git repository (see subprojects/*.wrap),
    # not as separate untracked checkouts. Each of those subprojects'
    # "maintainer-mode" option defaults to "if-git-build", detected via
    # `git ls-files meson.build` in its own source root -- which succeeds
    # here precisely because it's tracked in *our* repo, even though none
    # of them are being developed in maintainer mode. That flips their
    # "build-documentation" default on too, which pulls doxygen tag-file
    # generation for e.g. sigc++-3.0/cairomm-1.16/glibmm-2.68 in wherever
    # those are needed as *nested* subprojects (gtkmm -> cairomm,
    # gtkmm -> pangomm -> glibmm-2.68, etc). Since this project also vendors
    # its own top-level copies of those same dependencies, that nested,
    # nobody-asked-for doc build collides with the already-configured
    # top-level one. We don't need any of these libraries' documentation
    # built, so turn it off everywhere explicitly rather than relying on
    # each subproject's own default.
    'cairomm:build-documentation=false', 'cairomm:maintainer-mode=false',
    'gtkmm:build-documentation=false', 'gtkmm:maintainer-mode=false',
    'pangomm:build-documentation=false', 'pangomm:maintainer-mode=false',
    'glibmm-2.68:build-documentation=false', 'glibmm-2.68:maintainer-mode=false',
    'libsigcplusplus-3:build-documentation=false', 'libsigcplusplus-3:maintainer-mode=false',
  ],
)

app_id = 'io.github.chiddekel.Notepad'
gettext_package = meson.project_name()

gnome = import('gnome')
i18n = import('i18n')

gtkmm_dep = dependency('gtkmm-4.0', version: '>= 4.14',
  default_options: ['build-demos=false', 'build-tests=false', 'build-documentation=false'])
glibmm_dep = dependency('glibmm-2.68',
  default_options: ['build-examples=false', 'build-documentation=false'])
giomm_dep = dependency('giomm-2.68')
adwaita_dep = dependency('libadwaita-1', version: '>= 1.5',
  default_options: ['introspection=disabled', 'vapi=false', 'tests=false',
                     'examples=false', 'documentation=false'])

config_h = configuration_data()
config_h.set_quoted('APP_ID', app_id)
config_h.set_quoted('VERSION', meson.project_version())
config_h.set_quoted('GETTEXT_PACKAGE', gettext_package)
config_h.set_quoted('LOCALEDIR', get_option('prefix') / get_option('localedir'))
configure_file(output: 'config.h', configuration: config_h)
config_h_dir = include_directories('.')

subdir('src')
subdir('data')
subdir('po')
subdir('tests')
