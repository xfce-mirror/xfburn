dnl Version information
m4_define([xfburn_version_major], [0])
m4_define([xfburn_version_minor], [0])
m4_define([xfburn_version_micro], [2svn])
m4_define([xfburn_version], [xfburn_version_major().xfburn_version_minor().xfburn_version_micro()])

dnl Initialize autoconf
AC_COPYRIGHT([Copyright (c) 2005 Jean-François Wauthy <pollux@xfce.org>])
AC_INIT([xfburn], [xfburn_version()], [pollux@xfce.org])

dnl Initialize automake
AM_INIT_AUTOMAKE([AC_PACKAGE_TARNAME()], [AC_PACKAGE_VERSION()])
AM_CONFIG_HEADER([config.h])
AM_MAINTAINER_MODE()

dnl check for basic programs
AC_PROG_CC()
AC_PROG_INSTALL()
AC_PROG_LIBTOOL()

dnl check for libexo-0.3
XDT_CHECK_PACKAGE([EXO], [exo-0.3], [0.3.0])

dnl check for i18n support
XDT_I18N([fr])

dnl check for required packages
XDT_CHECK_PACKAGE([GTK], [gtk+-2.0], [2.6.0])
XDT_CHECK_PACKAGE([LIBXFCEGUI4], [libxfcegui4-1.0], [4.2.0])

dnl check for debugging support
BM_DEBUG_SUPPORT()

AC_OUTPUT([
Makefile
pixmaps/Makefile
po/Makefile.in
xfburn/Makefile
])
	
