ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

SUBDIRS = 								\
	desktop-integration                                             \
	icons								\
	po								\
	docs								\
	xfburn

uidir = $(datadir)/xfburn
ui_DATA =  	                                                        \
	xfburn.ui							\
	xfburn-popup-menus.ui							\
	xfburn-toolbars.ui

#
# appdata
#
@INTLTOOL_XML_RULE@
appdatadir = $(datadir)/metainfo
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
appdata_in_files = org.xfce.xfburn.appdata.xml.in

.PHONY: ChangeLog

ChangeLog: Makefile
	(GIT_DIR=$(top_srcdir)/.git git log > .changelog.tmp \
	&& mv .changelog.tmp ChangeLog; rm -f .changelog.tmp) \
	|| (touch ChangeLog; echo 'Git directory not found: installing possibly empty changelog.' >&2)

dist-hook: ChangeLog

EXTRA_DIST = 								\
	$(apps_in_files)						\
	intltool-extract.in						\
	intltool-merge.in						\
	intltool-update.in						\
	$(ui_DATA)							\
	$(appdata_in_files)						\
	README.md

appsdir = $(datadir)/applications
apps_in_files = xfburn.desktop.in
apps_DATA = $(apps_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

DISTCLEANFILES =							\
	intltool-extract						\
	intltool-merge							\
	intltool-update							\
	po/.intltool-merge-cache.lock					\
	$(apps_DATA)							\
	$(appdata_DATA)

# vi:set ts=8 sw=8 noet ai nocindent softtabstop=0 list listchars=tab\:>-:
