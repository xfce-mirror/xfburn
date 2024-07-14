dnl Version information
XDT_VERSION_INIT([0.7.1])

dnl Initialize autoconf
AC_COPYRIGHT([Copyright (c) 2005-2008 Jean-François Wauthy <pollux@xfce.org>, 2008 Mario Đanić <mario@libburnia-project.org>, 2008-2015 David Mohr <david@mcbf.net>, 2019-2020 Rene Kjellerup <rk.katana.steel@gmail.com>, 2022-2024 Hunter Turcin <huntertur@gmail.com>, Copyright (c) 2004-2024 Xfce development team. All rights reserved.])
AC_INIT([xfburn], [xdt_version], [https://gitlab.xfce.org/apps/xfburn])
AC_CONFIG_SRCDIR([xfburn/xfburn-main.c])
AC_PREREQ([2.69])
AC_REVISION([xdt_version_build])

dnl Initialize automake
AM_INIT_AUTOMAKE([1.11 no-dist-gzip dist-bzip2 tar-ustar foreign])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])
AM_MAINTAINER_MODE()

dnl check for basic programs
AC_PROG_CC()
AC_PROG_INSTALL()
LT_PREREQ([2.2.6])
LT_INIT()

AC_SYS_LARGEFILE

dnl check for i18n support
GETTEXT_PACKAGE="$PACKAGE"
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE], ["$GETTEXT_PACKAGE"], [Name of default gettext domain])
AC_SUBST([GETTEXT_PACKAGE])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])

dnl check for required packages
XDT_CHECK_PACKAGE([LIBBURN], [libburn-1], [0.4.2])
XDT_CHECK_PACKAGE([LIBISOFS], [libisofs-1], [0.6.2])
XDT_CHECK_PACKAGE([GTK], [gtk+-3.0], [3.20])
XDT_CHECK_PACKAGE([GLIB], [glib-2.0], [2.38])
XDT_CHECK_PACKAGE([LIBGIO], [gio-2.0], [2.38])
XDT_CHECK_PACKAGE([LIBGIOUNIX], [gio-unix-2.0], [2.38])
XDT_CHECK_PACKAGE([GTHREAD], [gthread-2.0], [2.38])
XDT_CHECK_PACKAGE([LIBXFCE4UI], [libxfce4ui-2], [4.12.0])
XDT_CHECK_PACKAGE([EXO], [exo-2], [0.11.0])
dnl **************************************
dnl *** Optional support for udev ***
dnl **************************************
XDT_CHECK_OPTIONAL_PACKAGE([GUDEV], [gudev-1.0], [145], [gudev], [UDEV glib support], [yes])
dnl **************************************
dnl *** Optional support for gstreamer ***
dnl **************************************
XDT_CHECK_OPTIONAL_PACKAGE([GST], [gstreamer-1.0], [1.0.0], [gstreamer], [GStreamer support for burning audio])
if test x"$GST_FOUND" = x"yes"; then
  XDT_CHECK_PACKAGE([GST_PBUTILS], [gstreamer-pbutils-1.0], [1.0])
fi


dnl check for debugging support
XDT_FEATURE_DEBUG([xdt_debug_default])

AC_CONFIG_FILES([
Makefile
icons/Makefile
icons/16x16/Makefile
icons/22x22/Makefile
icons/24x24/Makefile
icons/32x32/Makefile
icons/48x48/Makefile
icons/scalable/Makefile
po/Makefile.in
docs/Makefile
desktop-integration/Makefile
xfburn/Makefile
])
AC_OUTPUT
