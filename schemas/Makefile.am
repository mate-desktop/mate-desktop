@INTLTOOL_XML_NOMERGE_RULE@
gsettings_SCHEMAS = \
	org.mate.accessibility-keyboard.gschema.xml \
	org.mate.accessibility-startup.gschema.xml \
	org.mate.applications-at-mobility.gschema.xml \
	org.mate.applications-at-visual.gschema.xml \
	org.mate.applications-browser.gschema.xml \
	org.mate.applications-office.gschema.xml \
	org.mate.applications-terminal.gschema.xml \
	org.mate.background.gschema.xml \
	org.mate.file-views.gschema.xml \
	org.mate.interface.gschema.xml \
	org.mate.lockdown.gschema.xml \
	org.mate.peripherals-keyboard.gschema.xml \
	org.mate.peripherals-mouse.gschema.xml \
	org.mate.sound.gschema.xml \
	org.mate.thumbnail-cache.gschema.xml \
	org.mate.thumbnailers.gschema.xml \
	org.mate.typing-break.gschema.xml
@GSETTINGS_RULES@

%.gschema.xml.in: %.gschema.xml.in.in Makefile
	$(AM_V_GEN) $(SED) -e 's^\@datadir\@^$(datadir)^g' -e 's^\@GETTEXT_PACKAGE\@^$(GETTEXT_PACKAGE)^g' < $< > $@

convertdir = $(datadir)/MateConf/gsettings
convert_DATA = mate-desktop.convert

EXTRA_DIST = \
	$(convert_DATA) \
	$(gsettings_SCHEMAS)

CLEANFILES = \
	$(gsettings_SCHEMAS_in) \
	$(gsettings_SCHEMAS) \
	*.gschema.valid
