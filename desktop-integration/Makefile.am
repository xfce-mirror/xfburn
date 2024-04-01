desktopdir = $(datadir)/Thunar/sendto
desktop_in_files = thunar-sendto-xfburn.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

thunar-sendto-xfburn.desktop: thunar-sendto-xfburn.desktop.in
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

EXTRA_DIST =								\
	$(desktop_in_files)

DISTCLEANFILES =							\
	$(desktop_DATA)

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
