dnl $Id$

dnl Version information
m4_define([xfburn_version_major], [0])
m4_define([xfburn_version_minor], [0])
m4_define([xfburn_version_micro], [3svn])
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
AC_PROG_INTLTOOL([0.31], [no-xml])

dnl check for needed programs
AC_CHECK_PROG(cdrecord_found, [cdrecord], yes, no)
if test x"$cdrecord_found" = x"no"; then
  AC_MSG_ERROR([You need cdrecord to be able to write CDs with xfburn])
fi
AC_CHECK_PROG(readcd_found, [readcd], yes, no)
if test x"$readcd_found" = x"no"; then
  AC_MSG_ERROR([You need readcd to be able to create ISOs with xfburn])
fi
AC_CHECK_PROG(cdrdao_found, [cdrdao], yes, no)
if test x"$cdrdao_found" = x"no"; then
  AC_MSG_ERROR([You need cdrdao to be able to copy CDs with xfburn])
fi
AC_CHECK_PROG(mkisofs_found, [mkisofs], yes, no)
if test x"$mkisofs_found" = x"no"; then
  AC_MSG_ERROR([You need mkisofs to be able to create ISOs with xfburn])
fi

dnl check for i18n support
XDT_I18N([fr])

dnl check for required packages
XDT_CHECK_PACKAGE([GTK], [gtk+-2.0], [2.6.0])
XDT_CHECK_PACKAGE([LIBXFCEGUI4], [libxfcegui4-1.0], [4.2.0])
XDT_CHECK_PACKAGE([EXO], [exo-0.3], [0.3.0])

dnl check for debugging support
BM_DEBUG_SUPPORT()

REVISION=`LC_ALL=C svn info | head -n 4 | tail -n 1 | cut -d " " -f 2`
AC_DEFINE_UNQUOTED([REVISION], "${REVISION}", [svn revision number])
AC_SUBST([REVISION])

AC_OUTPUT([
Makefile
pixmaps/Makefile
po/Makefile.in
xfburn/Makefile
])
	
