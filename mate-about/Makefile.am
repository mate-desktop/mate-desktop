bin_PROGRAMS = mate-about
mate_about_SOURCES = mate-about.c mate-about.h
mate_about_CFLAGS = $(MATE_ABOUT_CFLAGS) $(UNIQUE_CFLAGS)
mate_about_LDADD = $(MATE_ABOUT_LIBS) $(UNIQUE_LIBS)

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	$(MATE_ABOUT_CFLAGS) \
	-DLOCALE_DIR=\"$(datadir)/locale\" \
	-DPIXMAPS_DIR=\"$(datadir)/pixmaps/\"

@INTLTOOL_DESKTOP_RULE@

if GNUCAT_ENABLED
pixmapdir  = $(datadir)/pixmaps
pixmap_DATA = gnu-cat_navideno_v3.png gnu-cat.gif
endif

desktopdir = $(datadir)/applications
desktop_in_files = mate-about.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

CLEANFILES = mate-about.desktop mate-about

#-include $(top_srcdir)/git.mk

