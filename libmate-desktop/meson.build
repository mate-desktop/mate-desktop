libmdt_inc = include_directories('.')

headers = files(
  'mate-desktop.h',
  'mate-desktop-utils.h',
  'mate-desktop-item.h',
  'mate-dconf.h',
  'mate-gsettings.h',
  'mate-bg.h',
  'mate-bg-crossfade.h',
  'mate-desktop-thumbnail.h',
  'mate-rr.h',
  'mate-languages.h',
  'mate-rr-config.h',
  'mate-rr-labeler.h',
  'mate-colorbutton.h',
  'mate-colorsel.h',
  'mate-hsv.h',
  'mate-colorseldialog.h',
)

install_headers(
  headers,
  subdir: 'mate-desktop-2.0/libmate-desktop',
)

sources = files(
  'mate-desktop-item.c',
  'mate-desktop-utils.c',
  'mate-desktop-thumbnail.c',
  'mate-dconf.c',
  'mate-gsettings.c',
  'mate-bg.c',
  'mate-bg-crossfade.c',
  'display-name.c',
  'mate-rr.c',
  'mate-languages.c',
  'mate-rr-config.c',
  'mate-rr-output-info.c',
  'mate-rr-labeler.c',
  'mate-colorbutton.c',
  'mate-colorsel.c',
  'mate-hsv.c',
  'mate-colorseldialog.c',
  'edid-parse.c',
)

deps = [
  x11_dep,
  gtk_dep,
  m_dep,
  libmdt_dep,
  dconf_dep,
]

if get_option('pnp-ids-path') == 'internal'
  pnp_ids_path = join_paths(matedt_pkgdatadir, 'pnp.ids')
  install_data('pnp.ids', install_dir: matedt_pkgdatadir)
else
  pnp_ids_path = get_option('pnp-ids-path')
endif

cflags = [
  '-DMATELOCALEDIR="@0@"'.format(matedt_localedir),
  '-DG_LOG_DOMAIN="MateDesktop"',
  '-DISO_CODES_PREFIX="@0@"'.format(iso_codes_prefix),
  '-DPNP_IDS="@0@"'.format(pnp_ids_path),
]

symbol_map = join_paths(meson.current_source_dir(), meson.project_name() + '.map')

test_ldflag = '-Wl,--version-script,' + symbol_map
ldflags = []

if cc.has_link_argument(test_ldflag)
  ldflags += test_ldflag
endif

libmate_desktop = shared_library(
  'mate-desktop-2',
  sources: sources,
  version: libversion,
  include_directories: top_inc,
  dependencies: deps,
  c_args: cflags,
  link_args: ldflags,
  link_depends: symbol_map,
  install: true,
  install_dir: matedt_libdir,
)

libmate_desktop_dep = declare_dependency(
  link_with: libmate_desktop,
  include_directories: libmdt_inc,
  dependencies: deps,
)

pkg.generate(
  libraries: libmate_desktop,
  version: matedt_version,
  name: matedt_api_name,
  description: 'Utility library for loading .desktop files',
  filebase: matedt_api_name,
  subdirs: matedt_api_name,
  requires: [
    'gtk+-3.0',
  ],
  requires_private: 'dconf',
  variables: 'exec_prefix=' + matedt_libexecdir,
  install_dir: join_paths(get_option('libdir'), 'pkgconfig')
)

if enable_gir
  gir_sources = sources + headers

  gir_incs = [
    'GObject-2.0',
    'Gtk-3.0',
  ]

  gir_extra_args = [
    '--warn-all',
    '--add-include-path=@0@'.format(meson.current_source_dir()),
    '-DMATE_DESKTOP_USE_UNSTABLE_API',
  ]

  gir_dir = join_paths(matedt_datadir, 'gir-1.0')
  typelib_dir = join_paths(matedt_libdir, 'girepository-1.0')

  gnome.generate_gir(
    libmate_desktop,
    sources: gir_sources,
    nsversion: matedt_api_version,
    namespace: matedt_gir_ns,
    symbol_prefix: 'mate_',
    identifier_prefix: 'Mate',
    export_packages: matedt_api_name,
    includes: gir_incs,
    extra_args: gir_extra_args,
    install: true,
    install_dir_gir: gir_dir,
    install_dir_typelib: typelib_dir,
  )
endif

test_ditem = executable('test-ditem',
  sources : 'test-ditem.c',
  dependencies : libmate_desktop_dep,
  install : false,
)

test_color_button = executable('test-color-button',
  sources : 'test.c',
  dependencies : libmate_desktop_dep,
  install : false,
)

test_languages = executable('test-languages',
  sources : 'test-languages.c',
  dependencies : libmate_desktop_dep,
  install : false,
)
