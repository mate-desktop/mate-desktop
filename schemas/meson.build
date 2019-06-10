schemas_data = configuration_data()
schemas_data.set('GETTEXT_PACKAGE', meson.project_name())
schemas_data.set('datadir', matedt_datadir)

schemas_in = [
  'org.mate.background.gschema.xml',
]

schemas = []
foreach s: schemas_in
  schemas += configure_file(
    input: '@0@.in'.format(s),
    output: '@0@'.format(s),
    configuration: schemas_data,
  )
endforeach

schemas += [
  'org.mate.accessibility-keyboard.gschema.xml',
  'org.mate.accessibility-startup.gschema.xml',
  'org.mate.applications-at-mobility.gschema.xml',
  'org.mate.applications-at-visual.gschema.xml',
  'org.mate.applications-browser.gschema.xml',
  'org.mate.applications-calculator.gschema.xml',
  'org.mate.applications-messenger.gschema.xml',
  'org.mate.applications-office.gschema.xml',
  'org.mate.applications-terminal.gschema.xml',
  'org.mate.debug.gschema.xml',
  'org.mate.file-views.gschema.xml',
  'org.mate.interface.gschema.xml',
  'org.mate.lockdown.gschema.xml',
  'org.mate.peripherals-keyboard.gschema.xml',
  'org.mate.peripherals-mouse.gschema.xml',
  'org.mate.sound.gschema.xml',
  'org.mate.thumbnail-cache.gschema.xml',
  'org.mate.thumbnailers.gschema.xml',
  'org.mate.typing-break.gschema.xml',
]

install_data(schemas,
  install_dir: join_paths(get_option('datadir'), 'glib-2.0/schemas'),
)
