INCLUDES = 								\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" 				\
	-DEXO_API_SUBJECT_TO_CHANGE					\
	-DDATADIR=\"$(datadir)\"					\
	-I$(top_srcdir)

bin_PROGRAMS = xfburn

xfburn_SOURCES =							\
	xfburn-main.c 							\
	xfburn-mainwindow.c 						\
	xfburn-filebrowser.c						

xfburn_CFLAGS =								\
	$(LIBXFCEGUI4_CFLAGS)						\
	$(EXO_CFLAGS)

xfburn_LDFLAGS = 							\
	$(LIBXFCEGUI4_LIBS)						\
	 $(EXO_LIBS)

