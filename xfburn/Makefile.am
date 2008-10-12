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
	xfburn-error.h							\
	xfburn-adding-progress.h					\
	xfburn-blank-dialog.h						\
	xfburn-perform-burn.h						\
	xfburn-burn-data-composition-base-dialog.h			\
	xfburn-burn-data-cd-composition-dialog.h			\
	xfburn-burn-data-dvd-composition-dialog.h			\
	xfburn-burn-audio-cd-composition-dialog.h			\
        xfburn-transcoder.h                                             \
        xfburn-transcoder-basic.h                                       \
	xfburn-burn-image-dialog.h					\
	xfburn-settings.h						\
	xfburn-composition.h						\
	xfburn-compositions-notebook.h					\
	xfburn-create-iso-progress-dialog.h				\
	xfburn-hal-manager.h						\
	xfburn-device-box.h						\
	xfburn-device-list.h						\
	xfburn-progress-dialog.h					\
	xfburn-main-window.h						\
	xfburn-notebook-tab.h						\
	xfburn-file-browser.h						\
	xfburn-welcome-tab.h						\
	xfburn-data-composition.h					\
	xfburn-audio-composition.h					\
	xfburn-disc-usage.h						\
	xfburn-data-disc-usage.h					\
	xfburn-audio-disc-usage.h					\
	xfburn-fs-browser.h						\
	xfburn-directory-browser.h					\
	xfburn-preferences-dialog.h					\
	xfburn-stock.h							\
	xfburn-utils.h							\
	xfburn-main.h
	
xfburn_SOURCES =							\
	$(xfburn_headers)						\
	xfburn-error.c          					\
	xfburn-adding-progress.c					\
	xfburn-blank-dialog.c						\
	xfburn-perform-burn.c						\
	xfburn-burn-data-composition-base-dialog.c			\
	xfburn-burn-data-cd-composition-dialog.c			\
	xfburn-burn-data-dvd-composition-dialog.c			\
	xfburn-burn-audio-cd-composition-dialog.c			\
        xfburn-transcoder.c                                             \
        xfburn-transcoder-basic.c                                       \
	xfburn-burn-image-dialog.c					\
	xfburn-composition.c						\
	xfburn-compositions-notebook.c					\
	xfburn-settings.c						\
	xfburn-hal-manager.c						\
	xfburn-device-box.c						\
	xfburn-device-list.c						\
	xfburn-progress-dialog.c					\
	xfburn-main.c 							\
	xfburn-main-window.c 						\
	xfburn-notebook-tab.c						\
	xfburn-file-browser.c						\
	xfburn-welcome-tab.c    	                             	\
	xfburn-data-composition.c    	                             	\
	xfburn-audio-composition.c    	                             	\
	xfburn-disc-usage.c						\
	xfburn-data-disc-usage.c					\
	xfburn-audio-disc-usage.c					\
	xfburn-fs-browser.c 	                                       	\
	xfburn-directory-browser.c					\
	xfburn-preferences-dialog.c					\
	xfburn-stock.c							\
	xfburn-utils.c	

xfburn_CFLAGS =								\
	$(GTHREAD_CFLAGS)						\
	$(LIBBURN_CFLAGS)						\
	$(LIBISOFS_CFLAGS)						\
	$(LIBXFCEGUI4_CFLAGS)						\
	$(EXO_CFLAGS)							\
	$(THUNAR_VFS_CFLAGS)						\
	$(GST_CFLAGS)

xfburn_LDADD = 								\
	$(GTHREAD_LIBS)							\
	$(LIBBURN_LIBS)							\
	$(LIBISOFS_LIBS)						\
	$(LIBXFCEGUI4_LIBS)						\
	$(EXO_LIBS)							\
	$(THUNAR_VFS_LIBS)						\
	$(GST_LIBS)

if HAVE_HAL
xfburn_CFLAGS +=							\
	$(HAL_CFLAGS)

xfburn_LDADD += 							\
	$(HAL_LIBS)
	$(HAL_DBUS_LIBS)

endif

# remove these when they're added to the build; needed
# for make distcheck (--brian)
missing_files = \
	xfburn-copy-cd-dialog.c \
	xfburn-copy-cd-progress-dialog.c \
	xfburn-copy-dvd-dialog.c					\
	xfburn-copy-cd-dialog.h						\
	xfburn-copy-cd-progress-dialog.h				\
	xfburn-copy-dvd-dialog.h					\
	xfburn-format-dvd-dialog.c					\
	xfburn-format-dvd-dialog.h					\
	xfburn-create-iso-progress-dialog.c

EXTRA_DIST = $(missing_files)
