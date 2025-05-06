ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

SUBDIRS = 								\
	desktop-integration                                             \
	icons								\
	docs								\
	xfburn								\
	po

uidir = $(datadir)/xfburn
ui_DATA =  	                                                        \
	xfburn.ui							\
	xfburn-popup-menus.ui							\
	xfburn-toolbars.ui

#
# appdata
#
org.xfce.xfburn.appdata.xml: org.xfce.xfburn.appdata.xml.in
	$(AM_V_GEN)$(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

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
	$(ui_DATA)								\
	$(appdata_in_files)						\
	meson_options.txt						\
	meson.build								\
	xfce-revision.h.in

appsdir = $(datadir)/applications
apps_in_files = xfburn.desktop.in
apps_DATA = $(apps_in_files:.desktop.in=.desktop)

xfburn.desktop: xfburn.desktop.in
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

DISTCLEANFILES =							\
	$(apps_DATA)							\
	$(appdata_DATA)

# vi:set ts=8 sw=8 noet ai nocindent softtabstop=0 list listchars=tab\:>-:
