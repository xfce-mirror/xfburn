INCLUDES = 								\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" 				\
	-DEXO_API_SUBJECT_TO_CHANGE					\
	-DEXO_DISABLE_DEPRECATED					\
	-DDATADIR=\"$(datadir)\"					\
	-I$(top_srcdir)

bin_PROGRAMS = 								\
	xfburn

xfburn_headers = 							\
	xfburn-global.h							\
	xfburn-adding-progress.h					\
	xfburn-blank-cd-dialog.h					\
	xfburn-burn-data-composition-dialog.h				\
	xfburn-burn-image-dialog.h					\
	xfburn-settings.h						\
	xfburn-composition.h						\
	xfburn-compositions-notebook.h					\
	xfburn-copy-cd-dialog.h						\
	xfburn-copy-cd-progress-dialog.h				\
	xfburn-copy-dvd-dialog.h					\
	xfburn-create-iso-progress-dialog.h				\
	xfburn-device-box.h						\
	xfburn-device-list.h						\
	xfburn-format-dvd-dialog.h					\
	xfburn-progress-dialog.h					\
	xfburn-main-window.h						\
	xfburn-notebook-tab.h						\
	xfburn-file-browser.h						\
	xfburn-data-composition.h					\
	xfburn-data-disc-usage.h					\
	xfburn-fs-browser.h						\
	xfburn-directory-browser.h					\
	xfburn-preferences-dialog.h					\
	xfburn-stock.h							\
	xfburn-utils.h	
	
xfburn_SOURCES =							\
	$(xfburn_headers)						\
	xfburn-adding-progress.c					\
	xfburn-blank-cd-dialog.c					\
	xfburn-burn-data-composition-dialog.c				\
	xfburn-burn-image-dialog.c					\
	xfburn-composition.c						\
	xfburn-compositions-notebook.c					\
	xfburn-settings.c						\
	xfburn-copy-dvd-dialog.c					\
	xfburn-device-box.c						\
	xfburn-device-list.c						\
	xfburn-format-dvd-dialog.c					\
	xfburn-progress-dialog.c					\
	xfburn-main.c 							\
	xfburn-main-window.c 						\
	xfburn-notebook-tab.c						\
	xfburn-file-browser.c						\
	xfburn-data-composition.c    	                             	\
	xfburn-data-disc-usage.c					\
	xfburn-fs-browser.c 	                                       	\
	xfburn-directory-browser.c					\
	xfburn-preferences-dialog.c					\
	xfburn-stock.c							\
	xfburn-utils.c	

xfburn_CFLAGS =								\
	$(LIBBURN_CFLAGS)						\
	$(LIBISOFS_CFLAGS)						\
	$(LIBXFCEGUI4_CFLAGS)						\
	$(EXO_CFLAGS)							\
	$(THUNAR_VFS_CFLAGS)						

xfburn_LDADD = 							\
	$(LIBBURN_LIBS)							\
	$(LIBISOFS_LIBS)						\
	$(LIBXFCEGUI4_LIBS)						\
	$(EXO_LIBS)							\
	$(THUNAR_VFS_LIBS)

