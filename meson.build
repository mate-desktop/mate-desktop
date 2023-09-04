project(
  'mate-desktop',
  'c',
  version: '1.27.3',
  license: [ 'GPL-2', 'FDL-1.1', 'LGPL-2' ],
)

matedt_version = meson.project_version()
version_array = matedt_version.split('.')
mate_major_version = version_array[0].to_int()
mate_minor_version = version_array[1].to_int()
mate_micro_version = version_array[2].to_int()
mate_platform = mate_major_version
mate_minor = mate_minor_version
mate_micro = mate_micro_version

matedt_api_version = '2.0'
matedt_api_name = '@0@-@1@'.format(meson.project_name(), matedt_api_version)

matedt_gir_ns = 'MateDesktop'
matedt_gir_version = '2.0'

matedt_gettext_package = meson.project_name()

matedt_prefix = get_option('prefix')
matedt_bindir = join_paths(matedt_prefix, get_option('bindir'))
matedt_datadir = join_paths(matedt_prefix, get_option('datadir'))
matedt_includedir = join_paths(matedt_prefix, get_option('includedir'))
matedt_libdir = join_paths(matedt_prefix, get_option('libdir'))
matedt_libexecdir = join_paths(matedt_prefix, get_option('libexecdir'))
matedt_localedir = join_paths(matedt_prefix, get_option('localedir'))
matedt_mandir = join_paths(matedt_prefix, get_option('mandir'))

matedt_pkgdatadir = join_paths(matedt_datadir, 'libmate-desktop')

# options
enable_gir = get_option('introspection')
enable_gtk_doc = get_option('gtk-doc')
enable_startup_notification = get_option('startup-notification')

# Before making a release, the LT_VERSION string should be modified.
# The string is of the form C:R:A.
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to C+1:0:A+1
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to C+1:0:0
# - If the interface is the same as the previous version, change to C:R+1:A
current = 17
revision = 4
age = 1

libversion = '@0@.@1@.@2@'.format(current, age, revision)

cc = meson.get_compiler('c')
nm = find_program('nm')

config_h = configuration_data()

# i18n
config_h.set_quoted('GETTEXT_PACKAGE', matedt_gettext_package)
config_h.set_quoted('LOCALEDIR', matedt_localedir)

common_flags = [
  '-DHAVE_CONFIG_H',
  '-I' + meson.build_root(),
  '-DPACKAGE_VERSION="@0@"'.format(matedt_version),
]

add_project_arguments(common_flags, language: 'c')

gtk_dep = dependency('gtk+-3.0', version: '>= 3.22.0')
m_dep = cc.find_library('m', required: false)
dconf_dep = dependency('dconf', version: '>= 0.13.4')
x11_dep = dependency('x11', required: true)
randr_dep = dependency('xrandr', version: '>= 1.3', required: false)
config_h.set('HAVE_RANDR', randr_dep.found())
iso_codes = dependency('iso-codes')
iso_codes_prefix = iso_codes.get_pkgconfig_variable('prefix')
libstartup_dep = dependency('libstartup-notification-1.0', version: '>= 0.5',
    required: enable_startup_notification )
libmdt_dep = [
  dependency('gdk-pixbuf-2.0'),
  dependency('gobject-2.0'),
  dependency('glib-2.0', version: '>= 2.50.0'),
  dependency('gio-2.0', version: '>= 2.26.0'),
  libstartup_dep,
  randr_dep,
  iso_codes,
]

config_h.set('HAVE_STARTUP_NOTIFICATION', libstartup_dep.found())
gnome = import('gnome')
i18n = import('i18n')
pkg = import('pkgconfig')

po_dir = join_paths(meson.source_root(), 'po')

top_inc = include_directories('.')

subdir('icons')
subdir('libmate-desktop')
subdir('man')
if enable_gtk_doc
  subdir('docs/reference/mate-desktop')
endif
subdir('po')
subdir('tools')
subdir('schemas')
subdir('xdg-portal')
if get_option('mate-about')
  subdir('mate-about')
endif

configure_file(
  output: 'config.h',
  configuration: config_h,
)

meson.add_install_script('utils/post_install.py')

test('libmate-desktop symbols check',
  find_program('abi-check'),
  env: [
    'LIB=@0@'.format(libmate_desktop.full_path()),
    'NM=@0@'.format(nm.path()),
    'ABI=@0@/libmate-desktop/mate-desktop.abi'.format(meson.source_root())
  ]
)
