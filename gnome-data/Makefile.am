mimedir = $(datadir)/mime-info
mime_DATA = gnome.mime

convertdir = $(datadir)/type-convert
convert_DATA = type.convert

GAMES_GROUP = games

EXTRA_DIST = 			\
	gnome.soundlist 	\
	gtk-events.soundlist	\
	mime-magic 		\
	gnome.mime 		\
	paper.config		\
	type-convert

scoredir=$(DESTDIR)$(localstatedir)/games
sysconf_DATA=paper.config mime-magic

soundlistdir = $(sysconfdir)/sound/events
soundlist_DATA = gnome.soundlist gtk-events.soundlist

install-data-local:
	$(mkinstalldirs) $(scoredir)
	-chgrp $(GAMES_GROUP) $(scoredir) && chmod g=rwXs $(scoredir)
	$(top_builddir)/libgnome/gnome-gen-mimedb $(DESTDIR)$(sysconfdir)/mime-magic
