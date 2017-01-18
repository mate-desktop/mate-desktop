bin_PROGRAMS = mate-color-select
bin_SCRIPTS =

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

CLEANFILES = *.desktop *.desktop.in
