mimedir = $(datadir)/mime-info
mime_DATA = gnome.mime

EXTRA_DIST = 			\
	gnome.soundlist 	\
	gtk-events.soundlist	\
	mime-magic 		\
	gnome.mime 		\
	paper.config

scoredir=$(localstatedir)/games
sysconf_DATA=paper.config mime-magic

soundlistdir = $(sysconfdir)/sound/events
soundlist_DATA = gnome.soundlist gtk-events.soundlist

install-data-local:
	$(mkinstalldirs) $(scoredir)
	-chgrp games $(scoredir) && chmod g=rwXs $(scoredir)
	$(top_builddir)/libgnome/gnome-gen-mimedb $(sysconfdir)/mime-magic
