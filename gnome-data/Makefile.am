convertdir = $(datadir)/type-convert
convert_DATA = type.convert postscript.convert
convert_SCRIPTS = gnome-make-postscript-mimes

GAMES_GROUP = games

EXTRA_DIST = 			\
	gnome.soundlist 	\
	gtk-events.soundlist	\
	paper.config		\
	type.convert		\
	postscript.convert	\
	gnome-make-postscript-mimes

scoredir=$(localstatedir)/games
sysconf_DATA=paper.config

soundlistdir = $(sysconfdir)/sound/events
soundlist_DATA = gnome.soundlist gtk-events.soundlist

postscript.convert: gnome-make-postscript-mimes
	$(SHELL) $(srcdir)/gnome-make-postscript-mimes > postscript.convert

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(scoredir)
	-chgrp $(GAMES_GROUP) $(scoredir) && chmod g=rwXs $(DESTDIR)$(scoredir)

