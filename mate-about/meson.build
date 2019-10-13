date_exe = find_program('date')
mate_date = run_command(date_exe, '+%Y-%m-%d').stdout().strip()
mate_year = run_command(date_exe, '+%Y').stdout().strip()

mate_data = configuration_data()

mate_data.set('MATE_DATE', mate_date)
mate_data.set('MATE_DATE_COMMENT_START', '<!--')
mate_data.set('MATE_DATE_COMMENT_END', '-->')
mate_data.set('MATE_PLATFORM', mate_platform)
mate_data.set('MATE_MINOR', mate_minor)
mate_data.set('MATE_MICRO', mate_micro)

mate_about_data = configuration_data()
mate_about_data.set('RELEASE_YEAR', mate_year)

executable('mate-about',
  'mate-about.c',
  c_args: [
    '-DLOCALE_DIR="@0@"'.format(join_paths(get_option('prefix'), get_option('localedir'))),
    '-DPIXMAPS_DIR="@0@"'.format(join_paths(get_option('prefix'), get_option('datadir'), 'pixmaps')),
  ],
  dependencies : gtk_dep,
  include_directories: top_inc,
  install: true,
)

i18n.merge_file(
  input: 'mate-about.desktop.in',
  output: 'mate-about.desktop',
  type: 'desktop',
  po_dir: '../po',
  install: true,
  install_dir: join_paths(get_option('datadir'), 'applications'),
)

mate_version_xml_in = configure_file(
  input: 'mate-version.xml.in',
  output: 'mate-version.xml',
  configuration: mate_data,
  install: true,
  install_dir: join_paths(get_option('datadir'), 'mate-about'),
)

mate_about_h_in = configure_file(
  input: 'mate-about.h.in',
  output: 'mate-about.h',
  configuration: mate_about_data,
  install: false,
)
