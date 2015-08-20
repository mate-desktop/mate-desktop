m4_define([mate_platform], [1])
m4_define([mate_minor], [10])
m4_define([mate_micro], [2])

m4_define(mate_version, [mate_platform.mate_minor.mate_micro]),

AC_INIT([mate-desktop], mate_version, [https://github.com/mate-desktop/mate-desktop/])
AC_CONFIG_SRCDIR([libmate-desktop])

AM_INIT_AUTOMAKE([1.9 foreign no-dist-gzip dist-xz tar-ustar check-news])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_CONFIG_HEADERS([config.h])

AC_CONFIG_MACRO_DIR([m4])

dnl make sure we keep ACLOCAL_FLAGS around for maintainer builds to work
AC_SUBST(ACLOCAL_AMFLAGS, "\${ACLOCAL_FLAGS}")

# Before making a release, the LT_VERSION string should be modified.
# The string is of the form C:R:A.
# - If interfaces have been changed or added, but binary compatibility has
#   been preserved, change to C+1:0:A+1
# - If binary compatibility has been broken (eg removed or changed interfaces)
#   change to C+1:0:0
# - If the interface is the same as the previous version, change to C:R+1:A

LT_VERSION=18:4:1
AC_SUBST(LT_VERSION)

AM_MAINTAINER_MODE

IT_PROG_INTLTOOL([0.40.0])

AC_ISC_POSIX
AC_PROG_CC
AC_STDC_HEADERS
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG

MATE_PLATFORM=mate_platform
MATE_MINOR=mate_minor
MATE_MICRO=mate_micro
MATE_DATE=`date +"%Y-%m-%d"`

MATE_DATE=
MATE_DATE_COMMENT_START="<!--"
MATE_DATE_COMMENT_END="-->"

AC_SUBST(MATE_PLATFORM)
AC_SUBST(MATE_MINOR)
AC_SUBST(MATE_MICRO)
AC_SUBST(MATE_DATE)
AC_SUBST(MATE_DATE_COMMENT_START)
AC_SUBST(MATE_DATE_COMMENT_END)

AC_DEFINE(MATE_MAJOR, [mate_platform], [Define to the major version])
AC_DEFINE(MATE_MINOR, [mate_minor], [Define to the minor version])
AC_DEFINE(MATE_MICRO, [mate_micro], [Define to the micro version])

MATE_COMMON_INIT
MATE_DEBUG_CHECK
MATE_COMPILE_WARNINGS([maximum])
MATE_MAINTAINER_MODE_DEFINES

AC_ARG_ENABLE(deprecation_flags,
              [AC_HELP_STRING([--enable-deprecation-flags],
                              [use *_DISABLE_DEPRECATED flags @<:@default=no@:>@])],,
              [enable_deprecation_flags=no])

if test "x$enable_deprecation_flags" = "xyes"; then
   DISABLE_DEPRECATED_CFLAGS=$DISABLE_DEPRECATED
   AC_SUBST(DISABLE_DEPRECATED_CFLAGS)
fi

# Should we build mate-about? Useful for people who need to build
# two mate-desktop using two different versions of GTK+
AC_ARG_ENABLE([mate-about],
	[AS_HELP_STRING([--disable-mate-about],[do not build mate-about])],,
	[enable_mate_about=yes])
AM_CONDITIONAL(MATE_ABOUT_ENABLED, test "x$enable_mate_about" = "xyes")


# Should we build the docbook user-guide? Will be useful until
# the new mallard mate-user-guide is complete and translated
AC_ARG_ENABLE([user-guide],
	[AS_HELP_STRING([--disable-user-guide],[do not build user guide])],,
	[enable_user_guide=yes])
AM_CONDITIONAL(USER_GUIDE_ENABLED, test "x$enable_user_guide" = "xyes")


# Install tool to migrate 1.4 MateConf settings
AC_ARG_ENABLE([mate-conf-import],
	[AS_HELP_STRING([--enable-mate-conf-import],[install mate-conf-import tool])],,
	[enable_mate_conf_import=no])
AM_CONDITIONAL(MATE_CONF_IMPORT_ENABLED, test "x$enable_mate_conf_import" = "xyes")


# Install tool to paste text to paste.mate-desktop.org
AC_ARG_ENABLE([mpaste],
	[AS_HELP_STRING([--enable-mpaste],[install mpaste tool])],,
	[enable_mpaste=no])
AM_CONDITIONAL(MPASTE_ENABLED, test "x$enable_mpaste" = "xyes")


# Path to the pnp.ids file -- to know if we use one shipped with another
# package, or an internal file

AC_ARG_WITH(pnp-ids-path,
              [AC_HELP_STRING([--with-pnp-ids-path],
                              [Specify the path to pnp.ids @<:@default=(internal)@:>@])],,
                              [with_pnp_ids_path="\${pnpdatadir}/pnp.ids"])

AM_CONDITIONAL(USE_INTERNAL_PNP_IDS, test "x$with_pnp_ids_path" = "x\${pnpdatadir}/pnp.ids")
PNP_IDS=$with_pnp_ids_path
AC_SUBST(PNP_IDS)
if test "x$with_pnp_ids_path" = "x\${pnpdatadir}/pnp.ids"; then
        EXTERNAL_PNP_IDS="no (internal)"
else
        EXTERNAL_PNP_IDS="$with_pnp_ids_path"
fi

dnl If you add a version number here, you *must* add an AC_SUBST line for
dnl it too, or it will never make it into the spec file!

dnl ----------------------------------------------------------------------------
dnl GTK library version
dnl ----------------------------------------------------------------------------
GTK_API_VERSION=2.0
GTK_REQUIRED=2.24.0

AC_MSG_CHECKING([which gtk+ version to compile against])
AC_ARG_WITH([gtk],
  [AS_HELP_STRING([--with-gtk=2.0|3.0],[which gtk+ version to compile against (default: 2.0)])],
  [case "$with_gtk" in
     2.0|3.0) ;;
     *) AC_MSG_ERROR([invalid gtk version specified]) ;;
   esac],
  [with_gtk=2.0])
AC_MSG_RESULT([$with_gtk])

case "$with_gtk" in
  2.0) GTK_API_VERSION=2.0
       GTK_REQUIRED=2.24.0
       ;;
  3.0) GTK_API_VERSION=3.0
       GTK_REQUIRED=3.0.0
       ;;
esac

GDK_PIXBUF_REQUIRED=2.4.0
GLIB_REQUIRED=2.36.0
GIO_REQUIRED=2.26.0
XRANDR_REQUIRED=1.2

AC_SUBST(GTK_API_VERSION)
AC_SUBST(GLIB_REQUIRED)
AC_SUBST(GDK_PIXBUF_REQUIRED)
AC_SUBST(GIO_REQUIRED)
AC_SUBST(XRANDR_REQUIRED)

dnl ----------------------------------------------------------------------------
dnl DCONF
dnl ----------------------------------------------------------------------------

DCONF_NEW_REQUIRED=0.13.4
DCONF_OLD_REQUIRED=0.10.0

PKG_CHECK_MODULES([DCONF], [dconf >= $DCONF_NEW_REQUIRED],
   [AC_DEFINE([HAVE_DCONF_0_13], [1], [Use DCONF >= $DCONF_NEW_REQUIRED])],
   [PKG_CHECK_MODULES([DCONF], [dconf >= $DCONF_OLD_REQUIRED],
       [AC_DEFINE([HAVE_DCONF_0_10], [1], [Use DCONF $DCONF_OLD_REQUIRED])
    ])
])
AC_SUBST(DCONF_CFLAGS)
AC_SUBST(DCONF_LIBS)

dnl ----------------------------------------------------------------------------
dnl startup notification
dnl ----------------------------------------------------------------------------
STARTUP_NOTIFICATION_VERSION=0.5
STARTUP_NOTIFICATION_PACKAGE=

AC_ARG_ENABLE(startup-notification,
	AC_HELP_STRING([--disable-startup-notification], [disable startup notification support]),, enable_startup_notification=auto)

AC_MSG_CHECKING([Startup notification library >= $STARTUP_NOTIFICATION_VERSION])

if $PKG_CONFIG --atleast-version $STARTUP_NOTIFICATION_VERSION libstartup-notification-1.0; then
	have_startup_notification=yes
else
	have_startup_notification=no
fi

AC_MSG_RESULT($have_startup_notification)

if test x$enable_startup_notification = xyes; then
	have_startup_notification=yes
	echo "startup-notification support forced on"
elif test x$enable_startup_notification = xauto; then
	true
else
	have_startup_notification=no
fi

if test x$have_startup_notification = xyes; then
	echo "Building with libstartup-notification"
	STARTUP_NOTIFICATION_PACKAGE=libstartup-notification-1.0
	AC_DEFINE(HAVE_STARTUP_NOTIFICATION,, [Building with startup notification support])
else
	echo "Building without libstartup-notification"
fi

AC_SUBST(STARTUP_NOTIFICATION_PACKAGE)

GOBJECT_INTROSPECTION_CHECK([0.9.7])

dnl we need x11 for MateBG

PKG_CHECK_MODULES(XLIB, x11,
    X11_PACKAGE=x11,
    [X11_PACKAGE=
     AC_PATH_XTRA
     if test "x$no_x" = xyes; then
       AC_MSG_ERROR("no (requires X development libraries)")
     else
       XLIB_LIBS="$X_PRE_LIBS $X_LIBS -lX11 $X_EXTRA_LIBS"
       XLIB_CFLAGS=$X_CFLAGS
     fi])
AC_SUBST(X11_PACKAGE)
AC_SUBST(XLIB_CFLAGS)
AC_SUBST(XLIB_LIBS)

dnl Checks for X is RANDR extension

AC_MSG_CHECKING(for xrandr)
if $PKG_CONFIG --atleast-version $XRANDR_REQUIRED xrandr; then
  AC_MSG_RESULT(yes)
  AC_DEFINE(HAVE_RANDR, 1,
            [Define if the xrandr-$XRANDR_REQUIRED library is present])
  have_randr=yes
  RANDR_PACKAGE=xrandr
else
  AC_MSG_RESULT(no)
  have_randr=no
  RANDR_PACKAGE=
fi

AC_SUBST(RANDR_PACKAGE)

dnl pkg-config dependency checks

PKG_CHECK_MODULES(MATE_DESKTOP, gdk-pixbuf-2.0 >= $GDK_PIXBUF_REQUIRED gtk+-$GTK_API_VERSION >= $GTK_REQUIRED glib-2.0 >= $GLIB_REQUIRED gio-2.0 >= $GIO_REQUIRED $STARTUP_NOTIFICATION_PACKAGE $RANDR_PACKAGE)

PKG_CHECK_MODULES(MATE_ABOUT, gtk+-$GTK_API_VERSION >= $GTK_REQUIRED)
AC_SUBST(MATE_ABOUT_CFLAGS)
AC_SUBST(MATE_ABOUT_LIBS)

dnl yelp-tools stuff

YELP_HELP_INIT

dnl Language Support

GETTEXT_PACKAGE=mate-desktop
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",
		   [The gettext translation domain])
AC_SUBST(GETTEXT_PACKAGE)

AM_GLIB_GNU_GETTEXT

GLIB_GSETTINGS

dnl Do not use AC_PROG_AWK since we need the full pathname.
AC_PATH_PROGS(AWK, mawk gawk nawk awk, )
AC_PATH_PROGS(PERL, perl5 perl)

dnl define a MAINT-like variable REBUILD which is set if Perl
dnl and awk are found, so autogenerated sources can be rebuilt
AC_ARG_ENABLE(rebuilds, [  --disable-rebuilds      disable all source autogeneration rules],,enable_rebuilds=yes)
REBUILD=\#
if test "x$enable_rebuilds" = "xyes" && \
     test -n "$PERL" && \
     $PERL -e 'exit !($] >= 5.002)' > /dev/null 2>&1 && \
     test -n "$AWK" ; then
  REBUILD=
fi
AC_SUBST(REBUILD)

AC_SEARCH_LIBS([sqrt], [m])

# check for gtk-doc
GTK_DOC_CHECK([1.4])

AC_CONFIG_FILES([
Makefile
mate-about/Makefile
mate-about/mate-about.desktop.in
libmate-desktop/Makefile
libmate-desktop/mate-desktop-2.0.pc
libmate-desktop/mate-desktop-2.0-uninstalled.pc
docs/Makefile
docs/reference/Makefile
docs/reference/mate-desktop/Makefile
mate-version.xml.in
po/Makefile.in
schemas/Makefile
man/Makefile
tools/mate-color-select.desktop.in
tools/mate-conf-import.desktop.in
tools/Makefile
user-guide/Makefile
user-guide/mate-user-guide.desktop.in
])

AC_OUTPUT

dnl ---------------------------------------------------------------------------
dnl - Show summary
dnl ---------------------------------------------------------------------------

echo "
mate-desktop $VERSION
`echo mate-desktop $VERSION | sed "s/./=/g"`

    prefix:                       ${prefix}
    exec_prefix:                  ${exec_prefix}
    libdir:                       ${libdir}
    bindir:                       ${bindir}
    sbindir:                      ${sbindir}
    sysconfdir:                   ${sysconfdir}
    localstatedir:                ${localstatedir}
    datadir:                      ${datadir}
    source code location:         ${srcdir}
    compiler:                     ${CC}
    cflags:                       ${CFLAGS}
    Maintainer mode:              ${USE_MAINTAINER_MODE}
    Use *_DISABLE_DEPRECATED:     ${enable_deprecation_flags}

    Gtk+ version:                 ${GTK_API_VERSION}
    Build mate-about:             ${enable_mate_about}
    Build user guide:             ${enable_user_guide}
    Install mate-conf-import:     ${enable_mate_conf_import}
    Install mpaste:               ${enable_mpaste}
    Use external pnp.ids:         ${EXTERNAL_PNP_IDS}
    Startup notification support: ${have_startup_notification}
    XRandr support:               ${have_randr}
    Build introspection support:  ${found_introspection}
    Build gtk-doc documentation:  ${enable_gtk_doc}
"
