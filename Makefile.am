SUBDIRS = libmate-desktop man docs po schemas tools icons

if MATE_ABOUT_ENABLED
SUBDIRS += mate-about
endif

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

DISTCHECK_CONFIGURE_FLAGS = \
	--enable-gtk-doc \
	--enable-mpaste

if MATE_ABOUT_ENABLED
versiondir = $(datadir)/mate-about
version_DATA = mate-version.xml
endif

@INTLTOOL_XML_RULE@

EXTRA_DIST =			\
	autogen.sh		\
	COPYING-DOCS		\
	mate-version.xml.in.in

CLEANFILES = \
	$(version_DATA)

MAINTAINERCLEANFILES = \
	$(srcdir)/aclocal.m4 \
	$(srcdir)/config.guess \
	$(srcdir)/config.h.in \
	$(srcdir)/config.sub \
	$(srcdir)/depcomp \
	$(srcdir)/install-sh \
	$(srcdir)/ltmain.sh \
	$(srcdir)/missing \
	$(srcdir)/mkinstalldirs \
	`find "$(srcdir)" -type f -name Makefile.in -print` \
	$(srcdir)/configure \
	$(srcdir)/gtk-doc.make \
	$(srcdir)/m4/gtk-doc.m4 \
	$(srcdir)/m4/intltool.m4

# Build ChangeLog from GIT  history
ChangeLog:
	$(AM_V_GEN) if test -d $(top_srcdir)/.git; then \
		GIT_DIR="$(top_srcdir)/.git" git log --stat > $@; \
	fi

dist: ChangeLog

.PHONY: ChangeLog

-include $(top_srcdir)/git.mk
