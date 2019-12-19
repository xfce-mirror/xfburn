SUBDIRS = 								\
	desktop-integration                                             \
	icons								\
	po								\
	docs								\
	xfburn

AUTOMAKE_OPTIONS = dist-bzip2

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
	$(ui_DATA)								\
	$(appdata_in_files)

appsdir = $(datadir)/applications
apps_in_files = xfburn.desktop.in
apps_DATA = $(apps_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

dist-bz2: dist
	zcat $(PACKAGE)-$(VERSION).tar.gz | \
           bzip2 --best -c > $(PACKAGE)-$(VERSION).tar.bz2

distcheck-bz2: distcheck
	zcat $(PACKAGE)-$(VERSION).tar.gz | \
            bzip2 --best -c > $(PACKAGE)-$(VERSION).tar.bz2

DISTCLEANFILES =							\
	intltool-extract						\
	intltool-merge							\
	intltool-update							\
	$(apps_DATA)							\
	$(appdata_DATA)

# vi:set ts=8 sw=8 noet ai nocindent softtabstop=0 list listchars=tab\:>-:
