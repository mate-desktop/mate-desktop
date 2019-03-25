SUBDIRS = libmate-desktop man docs po schemas tools icons

if MATE_ABOUT_ENABLED
SUBDIRS += mate-about
endif

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

DISTCHECK_CONFIGURE_FLAGS = \
	--enable-gtk-doc \
	CFLAGS='-Wno-deprecated-declarations'

EXTRA_DIST =			\
	autogen.sh		\
	COPYING-DOCS

CLEANFILES =

MAINTAINERCLEANFILES = \
	$(srcdir)/aclocal.m4 \
	$(srcdir)/build-aux/config.guess \
	$(srcdir)/build-aux/config.sub \
	$(srcdir)/build-aux/depcomp \
	$(srcdir)/build-aux/install-sh \
	$(srcdir)/build-aux/ltmain.sh \
	$(srcdir)/build-aux/missing \
	$(srcdir)/config.h.in \
	$(srcdir)/configure \
	$(srcdir)/gtk-doc.make \
	$(srcdir)/m4/gtk-doc.m4 \
	$(srcdir)/m4/intltool.m4 \
	$(srcdir)/m4/libtool.m4 \
	$(srcdir)/m4/lt~obsolete.m4 \
	$(srcdir)/m4/ltoptions.m4 \
	$(srcdir)/m4/ltsugar.m4 \
	$(srcdir)/m4/ltversion.m4 \
	$(srcdir)/mkinstalldirs \
	`find "$(srcdir)" -type f -name Makefile.in -print`

# Build ChangeLog from GIT  history
ChangeLog:
	$(AM_V_GEN) if test -d $(top_srcdir)/.git; then \
		GIT_DIR="$(top_srcdir)/.git" git log --stat > $@; \
	fi

dist: ChangeLog

.PHONY: ChangeLog

-include $(top_srcdir)/git.mk
