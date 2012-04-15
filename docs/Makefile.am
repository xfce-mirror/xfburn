# $Id$
# Based on Thunar's docs/Makefile.am

man_MANS = 					\
	xfburn.1

if MAINTAINER_MODE
%.1: %.xml
	xsltproc http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<
endif

EXTRA_DIST =					\
	$(man_MANS)				\
	xfburn.xml

# vi:set ts=8 sw=8 noet ai nocindent softtabstop=0 list listchars=tab\:>- syntax=automake:
