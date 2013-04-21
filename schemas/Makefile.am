desktop_gschemas_in_in = \
	org.mate.accessibility-keyboard.gschema.xml.in.in \
	org.mate.accessibility-startup.gschema.xml.in.in \
	org.mate.applications-at-mobility.gschema.xml.in.in \
	org.mate.applications-at-visual.gschema.xml.in.in \
	org.mate.applications-browser.gschema.xml.in.in \
	org.mate.applications-office.gschema.xml.in.in \
	org.mate.applications-terminal.gschema.xml.in.in \
	org.mate.background.gschema.xml.in.in \
	org.mate.file-views.gschema.xml.in.in \
	org.mate.interface.gschema.xml.in.in \
	org.mate.lockdown.gschema.xml.in.in \
	org.mate.peripherals-keyboard.gschema.xml.in.in \
	org.mate.peripherals-mouse.gschema.xml.in.in \
	org.mate.sound.gschema.xml.in.in \
	org.mate.thumbnail-cache.gschema.xml.in.in \
	org.mate.thumbnailers.gschema.xml.in.in \
	org.mate.typing-break.gschema.xml.in.in

@INTLTOOL_XML_NOMERGE_RULE@

desktop_gschemas_in = $(desktop_gschemas_in_in:.xml.in.in=.xml.in)
gsettings_SCHEMAS = $(desktop_gschemas_in:.xml.in=.xml)

%.gschema.xml.in: %.gschema.xml.in.in Makefile
	$(AM_V_GEN) $(SED) -e 's^\@datadir\@^$(datadir)^g' -e 's^\@GETTEXT_PACKAGE\@^$(GETTEXT_PACKAGE)^g' < $< > $@

@GSETTINGS_RULES@

convertdir = $(datadir)/MateConf/gsettings
convert_DATA = mate-desktop.convert

EXTRA_DIST = \
	$(convert_DATA) \
	$(desktop_gschemas_in_in)

CLEANFILES = \
	$(desktop_gschemas_in) \
	$(gsettings_SCHEMAS) \
	*.gschema.valid
