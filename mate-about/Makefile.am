bin_PROGRAMS = mate-about
mate_about_SOURCES = mate-about.c mate-about.h
mate_about_CFLAGS = $(MATE_ABOUT_CFLAGS)
mate_about_LDADD = $(MATE_ABOUT_LIBS)

AM_CPPFLAGS = \
	-I$(top_srcdir) \
	$(MATE_ABOUT_CFLAGS) \
	-DLOCALE_DIR=\"$(datadir)/locale\" \
	-DPIXMAPS_DIR=\"$(datadir)/pixmaps/\"

@INTLTOOL_DESKTOP_RULE@

desktopdir = $(datadir)/applications
desktop_in_files = mate-about.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

versiondir = $(datadir)/mate-about
version_DATA = mate-version.xml

@INTLTOOL_XML_RULE@

EXTRA_DIST = $(desktop_in_files)	\
	     mate-version.xml.in

CLEANFILES = $(desktop_DATA) $(bin_PROGRAMS) $(version_DATA)

#-include $(top_srcdir)/git.mk

