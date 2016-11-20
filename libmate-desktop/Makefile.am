libmate_desktopdir = $(includedir)/mate-desktop-2.0/libmate-desktop
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

noinst_PROGRAMS = test-ditem test

CLEANFILES =

introspection_sources = \
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
	mate-rr-output-info.c	\
	mate-rr-labeler.c	\
	mate-colorbutton.c \
	mate-colorsel.c \
	mate-colorseldialog.c \
	edid-parse.c

libmate_desktop_2_la_SOURCES =		\
	$(introspection_sources)	\
	mate-desktop-item.c		\
	mate-rr-private.h		\
	edid.h				\
	private.h

libmate_desktop_2_la_LIBADD =	\
	$(XLIB_LIBS)		\
	$(MATE_DESKTOP_LIBS)	\
	$(DCONF_LIBS)

libmate_desktop_2_la_LDFLAGS = \
	-version-info $(LT_VERSION) \
	-export-symbols-regex "^mate_.*" \
	-no-undefined

test_SOURCES = \
	test.c

test_LDADD = \
	libmate-desktop-2.la		\
	$(MATE_DESKTOP_LIBS)

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

-include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS =
INTROSPECTION_SCANNER_ARGS = --add-include-path=$(srcdir) --warn-all
INTROSPECTION_COMPILER_ARGS = --includedir=$(srcdir)

if HAVE_INTROSPECTION

MateDesktop-2.0.gir: libmate-desktop-2.la
MateDesktop_2_0_gir_INCLUDES = GObject-2.0 Gtk-3.0
MateDesktop_2_0_gir_PACKAGES = gdk-pixbuf-2.0 glib-2.0 gobject-2.0 gio-2.0 gtk+-3.0
MateDesktop_2_0_gir_CFLAGS = -DMATE_DESKTOP_USE_UNSTABLE_API -I$(top_srcdir)
MateDesktop_2_0_gir_LIBS = libmate-desktop-2.la
MateDesktop_2_0_gir_FILES = $(introspection_sources) $(libmate_desktop_HEADERS)
MateDesktop_2_0_gir_SCANNERFLAGS = --identifier-prefix=Mate --symbol-prefix=mate_
INTROSPECTION_SCANNER_ARGS += $(patsubst %,--c-include='libmate-desktop/%',$(libmate_desktop_HEADERS))
INTROSPECTION_GIRS += MateDesktop-2.0.gir

girdir = $(datadir)/gir-1.0
gir_DATA = $(INTROSPECTION_GIRS)

typelibdir = $(libdir)/girepository-1.0
typelib_DATA = $(INTROSPECTION_GIRS:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif

EXTRA_DIST = \
	mate-desktop-2.0.pc.in                \
	mate-desktop-2.0-uninstalled.pc.in    \
	$(pnpdata_DATA_dist)

MAINTAINERCLEANFILES = \
	pnp.ids

-include $(top_srcdir)/git.mk
