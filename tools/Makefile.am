bin_PROGRAMS = mate-color-select
bin_SCRIPTS = mate-gsettings-toggle

AM_CPPFLAGS = \
	-I$(top_srcdir)

mate_color_select_SOURCES = \
	mate-color-select.c

mate_color_select_CFLAGS = \
	-DLOCALE_DIR=\"$(datadir)/locale\" \
	$(MATE_DESKTOP_CFLAGS)

mate_color_select_LDADD = \
	$(top_builddir)/libmate-desktop/libmate-desktop-2.la \
	$(MATE_DESKTOP_LIBS)

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_files = mate-color-select.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
if MATE_CONF_IMPORT_ENABLED
desktop_in_files += mate-conf-import.desktop.in
bin_SCRIPTS += mate-conf-import
endif

if MPASTE_ENABLED
bin_SCRIPTS += mpaste
endif

EXTRA_DIST = \
	mate-conf-import.desktop.in.in \
	mate-conf-import \
	mate-gsettings-toggle \
	mpaste

CLEANFILES = *.desktop *.desktop.in
