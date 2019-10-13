AM_CPPFLAGS = 								\
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\"				\
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
	xfburn-transcoder.h						\
	xfburn-audio-track.h						\
	xfburn-transcoder-basic.h					\
	xfburn-transcoder-gst.h						\
	xfburn-audio-track-gst.h					\
	xfburn-burn-image-dialog.h					\
	xfburn-settings.h						\
	xfburn-composition.h						\
	xfburn-compositions-notebook.h					\
	xfburn-create-iso-progress-dialog.h				\
	xfburn-udev-manager.h						\
	xfburn-device-box.h						\
	xfburn-device.h							\
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
	xfburn-cclosure-marshal.h					\
	xfburn-utils.h							\
	xfburn-main.h

xfburn_SOURCES =							\
	$(xfburn_headers)						\
	xfburn-error.c							\
	xfburn-adding-progress.c					\
	xfburn-blank-dialog.c						\
	xfburn-perform-burn.c						\
	xfburn-burn-data-composition-base-dialog.c			\
	xfburn-burn-data-cd-composition-dialog.c			\
	xfburn-burn-data-dvd-composition-dialog.c			\
	xfburn-burn-audio-cd-composition-dialog.c			\
	xfburn-transcoder.c						\
	xfburn-audio-track.c						\
	xfburn-transcoder-basic.c					\
	xfburn-transcoder-gst.c						\
	xfburn-audio-track-gst.c					\
	xfburn-burn-image-dialog.c					\
	xfburn-composition.c						\
	xfburn-compositions-notebook.c					\
	xfburn-settings.c						\
	xfburn-udev-manager.c						\
	xfburn-device-box.c						\
	xfburn-device.c							\
	xfburn-device-list.c						\
	xfburn-progress-dialog.c					\
	xfburn-main.c							\
	xfburn-main-window.c 						\
	xfburn-notebook-tab.c						\
	xfburn-file-browser.c						\
	xfburn-welcome-tab.c						\
	xfburn-data-composition.c					\
	xfburn-audio-composition.c					\
	xfburn-disc-usage.c						\
	xfburn-data-disc-usage.c					\
	xfburn-audio-disc-usage.c					\
	xfburn-fs-browser.c 						\
	xfburn-directory-browser.c					\
	xfburn-preferences-dialog.c					\
	xfburn-cclosure-marshal.c					\
	xfburn-utils.c

xfburn_CFLAGS =								\
	$(GTHREAD_CFLAGS)						\
	$(LIBBURN_CFLAGS)						\
	$(LIBISOFS_CFLAGS)						\
	$(LIBXFCE4UI_CFLAGS)						\
	$(EXO_CFLAGS)							\
	$(GIO_CFLAGS)							\
	$(GIO_UNIX_CFLAGS)						\
	$(GLIB_CFLAGS)							\
	$(GTHREAD_CFLAGS)						\
	$(GTK_CFLAGS)							\
	$(GST_CFLAGS)							\
	$(GST_PBUTILS_CFLAGS)

xfburn_LDADD = 								\
        -lm                                                             \
	$(GTHREAD_LIBS)							\
	$(LIBBURN_LIBS)							\
	$(LIBISOFS_LIBS)						\
	$(LIBXFCE4UI_LIBS)						\
	$(EXO_LIBS)							\
	$(GIO_LIBS)							\
	$(GIO_UNIX_LIBS)						\
	$(GLIB_LIBS)							\
	$(GTHREAD_LIBS)							\
	$(GTK_LIBS)							\
	$(GST_LIBS)							\
	$(GST_PBUTILS_LIBS)

if HAVE_GUDEV
xfburn_CFLAGS +=							\
	$(GUDEV_CFLAGS)

xfburn_LDADD += 							\
	$(GUDEV_LIBS)

endif

xfburn-cclosure-marshal.h: xfburn-cclosure-marshal.list
	glib-genmarshal --header --prefix=xfburn_cclosure_marshal $< > $@

xfburn-cclosure-marshal.c: xfburn-cclosure-marshal.list
	glib-genmarshal --body --prefix=xfburn_cclosure_marshal $< \
		| awk 'BEGIN { print "#include \"$(patsubst %.c,%.h, $@)\"" } { print; }' > $@


BUILT_SOURCES = 							\
	xfburn-cclosure-marshal.h					\
	xfburn-cclosure-marshal.c

# remove these when they're added to the build; needed
# for make distcheck (--brian)
missing_files = \
	xfburn-copy-cd-dialog.c \
	xfburn-copy-cd-progress-dialog.c \
	xfburn-copy-dvd-dialog.c					\
	xfburn-copy-cd-dialog.h						\
	xfburn-copy-cd-progress-dialog.h				\
	xfburn-copy-dvd-dialog.h					\
	xfburn-create-iso-progress-dialog.c

EXTRA_DIST = $(missing_files) \
	xfburn-cclosure-marshal.list

DISTCLEANFILES = $(BUILT_SOURCES)

# vi:set ts=8 sw=8 noet ai nocindent softtabstop=0 list listchars=tab\:>-:
