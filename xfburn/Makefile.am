INCLUDES = 								\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" 				\
	-DEXO_API_SUBJECT_TO_CHANGE					\
	-DDATADIR=\"$(datadir)\"					\
	-I$(top_srcdir)

bin_PROGRAMS = 								\
	xfburn

xfburn_headers = 							\
	xfburn-global.h							\
	xfburn-blank-cd-dialog.h					\
	xfburn-blank-cd-progress-dialog.h				\
	xfburn-burn-composition-dialog.h				\
	xfburn-burn-composition-progress-dialog.h			\
	xfburn-burn-image-dialog.h					\
	xfburn-burn-image-progress-dialog.h				\
	xfburn-settings.h						\
	xfburn-copy-cd-dialog.h						\
	xfburn-copy-cd-progress-dialog.h				\
	xfburn-create-iso-progress-dialog.h				\
	xfburn-create-iso-from-composition-progress-dialog.h		\
	xfburn-progress-dialog.h					\
	xfburn-main-window.h						\
	xfburn-file-browser.h						\
	xfburn-disc-content.h						\
	xfburn-disc-usage.h						\
	xfburn-fs-browser.h						\
	xfburn-directory-browser.h					\
	xfburn-preferences-dialog.h					\
	xfburn-utils.h
	
xfburn_SOURCES =							\
	$(xfburn_headers)						\
	xfburn-blank-cd-dialog.c					\
	xfburn-blank-cd-progress-dialog.c				\
	xfburn-burn-composition-dialog.c				\
	xfburn-burn-composition-progress-dialog.c			\
	xfburn-burn-image-dialog.c					\
	xfburn-burn-image-progress-dialog.c				\
	xfburn-create-iso-progress-dialog.c				\
	xfburn-create-iso-from-composition-progress-dialog.c		\
	xfburn-settings.c						\
	xfburn-copy-cd-dialog.c						\
	xfburn-copy-cd-progress-dialog.c				\
	xfburn-progress-dialog.c					\
	xfburn-main.c 							\
	xfburn-main-window.c 						\
	xfburn-file-browser.c						\
	xfburn-disc-content.c    	                             	\
	xfburn-disc-usage.c						\
	xfburn-fs-browser.c 	                                       	\
	xfburn-directory-browser.c					\
	xfburn-preferences-dialog.c					\
	xfburn-utils.c

xfburn_CFLAGS =								\
	$(LIBXFCEGUI4_CFLAGS)						\
	$(EXO_CFLAGS)

xfburn_LDFLAGS = 							\
	$(LIBXFCEGUI4_LIBS)						\
	 $(EXO_LIBS)

