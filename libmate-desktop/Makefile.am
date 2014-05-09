libmate_desktopdir = $(includedir)/mate-desktop-$(GTK_API_VERSION)/libmate-desktop
libmate_desktop_HEADERS = \
	mate-desktop.h \
	mate-desktop-utils.h \
	mate-desktop-item.h \
	mate-dconf.h \
	mate-gsettings.h \
	mate-aboutdialog.h \
	mate-bg.h			\
	mate-bg-crossfade.h		\
	mate-desktop-thumbnail.h	\
	mate-rr.h			\
	mate-rr-config.h		\
	mate-rr-labeler.h \
	mate-colorbutton.h \
	mate-colorsel.h \
	mate-colorseldialog.h

AM_CPPFLAGS =							\
	$(MATE_DESKTOP_CFLAGS)					\
	$(XLIB_CFLAGS)						\
	$(DCONF_CFLAGS)						\
	-DG_LOG_DOMAIN=\"MateDesktop\"				\
	-DMATELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale\""	\
	-DPNP_IDS=\""$(PNP_IDS)"\"				\
	$(DISABLE_DEPRECATED_CFLAGS)

AM_CFLAGS = $(WARN_CFLAGS)

noinst_PROGRAMS = test-ditem test

if USE_GTK3
lib_LTLIBRARIES=libmate-desktop-3.la

libmate_desktop_3_la_SOURCES = \
	mate-desktop-item.c	\
	mate-desktop-utils.c	\
	mate-desktop-thumbnail.c \
	mate-thumbnail-pixbuf-utils.c \
	mate-dconf.c \
	mate-gsettings.c \
	mate-aboutdialog.c \
	mate-bg.c		\
	mate-bg-crossfade.c	\
	display-name.c		\
	mate-rr.c		\
	mate-rr-config.c	\
	mate-rr-labeler.c	\
	mate-rr-private.h	\
	mate-colorbutton.c \
	mate-colorsel.c \
	mate-colorseldialog.c \
	edid-parse.c		\
	edid.h			\
	private.h

libmate_desktop_3_la_LIBADD =	\
	$(XLIB_LIBS)		\
	$(MATE_DESKTOP_LIBS)	\
	$(DCONF_LIBS)

libmate_desktop_3_la_LDFLAGS = \
	-version-info $(LT_VERSION) \
	-no-undefined
else
lib_LTLIBRARIES=libmate-desktop-2.la

libmate_desktop_2_la_SOURCES = \
	mate-desktop-item.c	\
	mate-desktop-utils.c	\
	mate-desktop-thumbnail.c \
	mate-thumbnail-pixbuf-utils.c \
	mate-dconf.c \
	mate-gsettings.c \
	mate-aboutdialog.c \
	mate-bg.c		\
	mate-bg-crossfade.c	\
	display-name.c		\
	mate-rr.c		\
	mate-rr-config.c	\
	mate-rr-labeler.c	\
	mate-rr-private.h	\
	mate-colorbutton.c \
	mate-colorsel.c \
	mate-colorseldialog.c \
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
endif


test_SOURCES = \
	test.c

if USE_GTK3
test_LDADD = \
	libmate-desktop-3.la		\
	$(MATE_DESKTOP_LIBS)

test_ditem_LDADD = \
	libmate-desktop-3.la		\
	$(XLIB_LIBS)			\
	$(MATE_DESKTOP_LIBS)
else
test_LDADD = \
	libmate-desktop-3.la		\
	$(MATE_DESKTOP_LIBS)

test_ditem_LDADD = \
	libmate-desktop-3.la		\
	$(XLIB_LIBS)			\
	$(MATE_DESKTOP_LIBS)
endif

test_ditem_SOURCES = \
	test-ditem.c


pkgconfigdir = $(libdir)/pkgconfig

if USE_GTK3
pkgconfig_DATA = mate-desktop-3.0.pc
else
pkgconfig_DATA = mate-desktop-2.0.pc
endif

if USE_INTERNAL_PNP_IDS
pnpdatadir = $(datadir)/libmate-desktop
pnpdata_DATA = pnp.ids
endif
pnpdata_DATA_dist = pnp.ids

check:
	test -s $(top_srcdir)/libmate-desktop/pnp.ids

if USE_GTK3
EXTRA_DIST = \
	mate-desktop-3.0.pc.in                \
	$(pnpdata_DATA_dist)
else
EXTRA_DIST = \
	mate-desktop-2.0.pc.in                \
	$(pnpdata_DATA_dist)
endif

MAINTAINERCLEANFILES = \
	pnp.ids

-include $(top_srcdir)/git.mk
