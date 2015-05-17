#!/bin/sh
#
# $Id: autogen.sh 13564 2005-04-12 15:03:11Z jasper $
#
# Copyright (c) 2002-2005
#         The Xfce development team. All rights reserved.
#
# Written for Xfce by Benedikt Meurer <benny@xfce.org>.
#

(type xdt-autogen) >/dev/null 2>&1 || {
  cat >&2 <<EOF
autogen.sh: You don't seem to have the Xfce development tools installed on
            your system, which are required to build this software.
            Please install the xfce4-dev-tools package first, it is available
            from http://www.xfce.org/.
EOF
  exit 1
}

# substitute revision and linguas
linguas=`ls po/*.po | awk 'BEGIN { FS="[./]"; ORS=" " } { print $2 }'`

if [ -d .git ]; then
  revision=`git rev-parse --short HEAD 2>/dev/null`
fi

if [ -z "$revision" ]; then
  revision="UNKNOWN"
fi

sed -e "s/@LINGUAS@/${linguas}/g" \
    -e "s/@REVISION@/${revision}/g" \
    < "configure.ac.in" > "configure.ac"

if test -f autogen.configureflags; then
  configureflags=`cat autogen.configureflags`
fi

exec xdt-autogen $configureflags $@

# vi:set ts=2 sw=2 et ai:
