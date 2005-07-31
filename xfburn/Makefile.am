INCLUDES = 								\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" 				\
	-DEXO_API_SUBJECT_TO_CHANGE					\
	-DDATADIR=\"$(datadir)\"					\
	-I$(top_srcdir)

bin_PROGRAMS = 								\
	xfburn

xfburn_headers = 							\
	xfburn-main-window.h						\
	xfburn-file-browser.h						\
	xfburn-disc-content.h						\
	xfburn-fs-browser.h						\
	xfburn-directory-browser.h					
	
xfburn_SOURCES =							\
	$(xfburn_headers)						\
	xfburn-main.c 							\
	xfburn-main-window.c 						\
	xfburn-file-browser.c						\
	xfburn-disc-content.c    	                             	\
	xfburn-fs-browser.c 	                                       	\
	xfburn-directory-browser.c

xfburn_CFLAGS =								\
	$(LIBXFCEGUI4_CFLAGS)						\
	$(EXO_CFLAGS)

xfburn_LDFLAGS = 							\
	$(LIBXFCEGUI4_LIBS)						\
	 $(EXO_LIBS)

