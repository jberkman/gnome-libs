scoredir=$(localstatedir)/games
sysconf_DATA=paper.config mime.types

soundlistdir = $(sysconfdir)/sound/events
soundlist_DATA = gnome.soundlist gtk-events.soundlist

install-data-local:
	$(mkinstalldirs) $(scoredir)
	-chgrp games $(scoredir) && chmod g=rwXs $(scoredir)
