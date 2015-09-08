desktop_gschemas_in = \
	org.mate.accessibility-keyboard.gschema.xml.in \
	org.mate.accessibility-startup.gschema.xml.in \
	org.mate.applications-at-mobility.gschema.xml.in \
	org.mate.applications-at-visual.gschema.xml.in \
	org.mate.applications-browser.gschema.xml.in \
	org.mate.applications-office.gschema.xml.in \
	org.mate.applications-terminal.gschema.xml.in \
	org.mate.background.gschema.xml.in \
	org.mate.debug.gschema.xml.in \
	org.mate.file-views.gschema.xml.in \
	org.mate.interface.gschema.xml.in \
	org.mate.lockdown.gschema.xml.in \
	org.mate.peripherals-keyboard.gschema.xml.in \
	org.mate.peripherals-mouse.gschema.xml.in \
	org.mate.sound.gschema.xml.in \
	org.mate.thumbnail-cache.gschema.xml.in \
	org.mate.thumbnailers.gschema.xml.in \
	org.mate.typing-break.gschema.xml.in

@INTLTOOL_XML_NOMERGE_RULE@

gsettings_SCHEMAS = $(desktop_gschemas_in:.xml.in=.xml)

@GSETTINGS_RULES@

convertdir = $(datadir)/MateConf/gsettings
convert_DATA = mate-desktop.convert

EXTRA_DIST = \
	$(convert_DATA) \
	$(desktop_gschemas_in)

CLEANFILES = \
	$(gsettings_SCHEMAS) \
	*.gschema.valid
