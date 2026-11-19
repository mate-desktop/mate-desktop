mate_wayland_protocols = files(
  'mate-input-device-v1.xml',
  'mate-output-v1.xml',
)

mate_wayland_protocols_dir = join_paths(get_option('datadir'), 'mate-wayland-protocols')

install_data(
  mate_wayland_protocols,
  install_dir: mate_wayland_protocols_dir,
)

pkg.generate(
  name: 'mate-wayland-protocols',
  description: 'MATE Wayland protocol definitions',
  version: matedt_version,
  dataonly: true,
  variables: 'pkgdatadir=${prefix}/' + mate_wayland_protocols_dir,
  install_dir: join_paths(get_option('datadir'), 'pkgconfig'),
)
