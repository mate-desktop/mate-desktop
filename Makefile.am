SUBDIRS =			\
	po 			\
	libmate-desktop 	\
	docs			\
	man

if MATE_ABOUT_ENABLED
SUBDIRS += mate-about
endif
if DESKTOP_DOCS_ENABLED
SUBDIRS += desktop-docs
endif

ACLOCAL_AMFLAGS = -I m4

if MATE_ABOUT_ENABLED
versiondir=$(datadir)/mate-about
version_DATA=mate-version.xml
endif
@INTLTOOL_XML_RULE@

EXTRA_DIST =			\
	COPYING-DOCS		\
	HACKING			\
	MAINTAINERS		\
	mate-version.xml.in.in	\
	mate-doc-utils.make

DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc --disable-scrollkeeper

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
	$(srcdir)/m4/mate-doc-utils.m4 \
	$(srcdir)/m4/gtk-doc.m4 \
	$(srcdir)/m4/intltool.m4

-include $(top_srcdir)/git.mk
