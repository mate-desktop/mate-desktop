libmate_desktopdir = $(includedir)/mate-desktop-2.0/
libmate_desktop_HEADERS = \
	mate-desktop-utils.h \
	mate-desktop-item.h \
	mate-dconf.h \
	mate-gsettings.h \
	mate-bg.h			\
	mate-bg-crossfade.h		\
	mate-desktop-thumbnail.h	\
	mate-rr.h			\
	mate-rr-config.h		\
	mate-rr-labeler.h

lib_LTLIBRARIES = libmate-desktop-2.la

AM_CPPFLAGS =							\
	$(MATE_DESKTOP_CFLAGS)					\
	$(XLIB_CFLAGS)						\
	$(DCONF_CFLAGS)						\
	-DG_LOG_DOMAIN=\"MateDesktop\"				\
	-DMATELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale\""	\
	-DPNP_IDS=\""$(PNP_IDS)"\"				\
	$(DISABLE_DEPRECATED_CFLAGS)

AM_CFLAGS = $(WARN_CFLAGS)

noinst_PROGRAMS = test-ditem

libmate_desktop_2_la_SOURCES = \
	mate-desktop-item.c	\
	mate-desktop-utils.c	\
	mate-desktop-thumbnail.c \
	mate-thumbnail-pixbuf-utils.c \
	mate-dconf.c \
	mate-gsettings.c \
	mate-bg.c		\
	mate-bg-crossfade.c	\
	display-name.c		\
	mate-rr.c		\
	mate-rr-config.c	\
	mate-rr-labeler.c	\
	mate-rr-private.h	\
	edid-parse.c		\
	edid.h			\
	private.h

libmate_desktop_2_la_LIBADD =	\
	$(XLIB_LIBS)		\
	$(MATE_DESKTOP_LIBS)	\
	$(DCONF_LIBS)

libmate_desktop_2_la_LDFLAGS = \
	-version-info $(LT_VERSION) \
	-no-undefined

test_ditem_SOURCES = \
	test-ditem.c

test_ditem_LDADD = \
	libmate-desktop-2.la		\
	$(XLIB_LIBS)			\
	$(MATE_DESKTOP_LIBS)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mate-desktop-2.0.pc

if USE_INTERNAL_PNP_IDS
pnpdatadir = $(datadir)/libmate-desktop
pnpdata_DATA = pnp.ids
endif
pnpdata_DATA_dist = pnp.ids

check:
	test -s $(top_srcdir)/libmate-desktop/pnp.ids

EXTRA_DIST = \
	mate-desktop-2.0.pc.in                \
	mate-desktop-2.0-uninstalled.pc.in    \
	$(pnpdata_DATA_dist)

MAINTAINERCLEANFILES = \
	pnp.ids

-include $(top_srcdir)/git.mk
