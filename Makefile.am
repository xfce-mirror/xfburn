SUBDIRS = 								\
	pixmaps								\
	po 								\
    	xfburn

AUTOMAKE_OPTIONS = dist-bzip2

uidir = $(datadir)/xfburn
ui_DATA =  	                                                        \
        xfburn.ui							\
	xfburn-toolbars.ui
		
EXTRA_DIST = 								\
	$(apps_in_files)						\
	intltool-extract.in						\
	intltool-merge.in						\
	intltool-update.in						\
	$(ui_DATA)

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
