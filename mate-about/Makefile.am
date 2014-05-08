bin_PROGRAMS = mate-about
mate_about_SOURCES = mate-about.c mate-about.h
mate_about_CFLAGS = $(MATE_ABOUT_CFLAGS) $(UNIQUE_CFLAGS)
if USE_GTK3
mate_about_LDADD = $(MATE_ABOUT_LIBS) $(UNIQUE_LIBS) $(top_builddir)/libmate-desktop/libmate-desktop-3.la
else
mate_about_LDADD = $(MATE_ABOUT_LIBS) $(UNIQUE_LIBS) $(top_builddir)/libmate-desktop/libmate-desktop-2.la
endif

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	$(MATE_ABOUT_CFLAGS) \
	-DLOCALE_DIR=\"$(datadir)/locale\" \
	-DPIXMAPS_DIR=\"$(datadir)/pixmaps/\"

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_files = mate-about.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

CLEANFILES = mate-about.desktop mate-about

#-include $(top_srcdir)/git.mk

