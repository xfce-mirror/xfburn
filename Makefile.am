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
	$(ui_DATA)

appsdir = $(datadir)/applications
apps_in_files = xfburn.desktop.in
apps_DATA = $(apps_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@
