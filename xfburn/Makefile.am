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
	xfburn-burn-image-dialog.h					\
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
	xfburn-burn-image-dialog.c					\
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

