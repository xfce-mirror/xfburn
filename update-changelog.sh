#!/bin/bash

USRBIN=/usr/bin/svn2cl

if [ -x $USRBIN ]; then
  BIN=$USRBIN
else
  BIN=svn2cl.sh
fi

$BIN --group-by-day --reparagraph --break-before-msg --authors=committers.xml

