bin_SCRIPTS = mate-gsettings-toggle

if MATE_CONF_IMPORT_ENABLED
@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_files = mate-conf-import.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
bin_SCRIPTS += mate-conf-import
endif

if MPASTE_ENABLED
bin_SCRIPTS += mpaste
endif

EXTRA_DIST = \
	mate-conf-import \
	mate-gsettings-toggle \
	mpaste

CLEANFILES = mate-conf-import.desktop mate-conf-import.desktop.in
