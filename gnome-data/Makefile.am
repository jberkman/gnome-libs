mimedir = $(datadir)/mime-info
mime_DATA = gnome.mime


convertdir = $(datadir)/type-convert
convert_DATA = type.convert postscript.convert
convert_SCRIPTS = gnome-make-postscript-mimes

GAMES_GROUP = games

EXTRA_DIST = 			\
	gnome.soundlist 	\
	gtk-events.soundlist	\
	mime-magic 		\
	gnome.mime 		\
	paper.config		\
	type.convert		\
	postscript.convert	\
	gnome-make-postscript-mimes \
	$(m4data_DATA)

scoredir=$(DESTDIR)$(localstatedir)/games
sysconf_DATA=paper.config mime-magic

soundlistdir = $(sysconfdir)/sound/events
soundlist_DATA = gnome.soundlist gtk-events.soundlist

postscript.convert: gnome-make-postscript-mimes
	$(SHELL) $(srcdir)/gnome-make-postscript-mimes > postscript.convert

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(scoredir)
	-chgrp $(GAMES_GROUP) $(scoredir) && chmod g=rwXs $(DESTDIR)$(scoredir)
	$(top_builddir)/libgnome/gnome-gen-mimedb $(DESTDIR)$(sysconfdir)/mime-magic

m4datadir = $(datadir)/aclocal
m4data_DATA=compiler-flags.m4 gnome-common.m4 gnome-cxx-check.m4 \
	gnome-fileutils.m4 gnome-gettext.m4 gnome-gnorba-check.m4 \
	gnome-pthread-check.m4 gnome-support.m4 \
	gnome-x-checks.m4 gnome.m4 linger.m4 need-declaration.m4 \
	aclocal-include.m4
