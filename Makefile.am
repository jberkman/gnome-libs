built_SUBDIRS = libgnome libgnomeui gtk-xmhtml gnome-hello libgtktty

SUBDIRS = po intl $(built_SUBDIRS)

DISTCLEANFILES = stamp-v

include_HEADERS = gnome.h

EXTRA_DIST = HACKING

release:
	$(MAKE) disk distdir=$(PACKAGE)$(VERSION)

install-data-local:
	$(INSTALL_DATA) $(srcdir)/gnomeConf.sh $(prefix)/lib
	echo >> $(prefix)/lib/gnomeConf.sh
	echo "GNOME_LIBDIR=\"-L$(libdir)\"" >> $(prefix)/lib/gnomeConf.sh
	echo "GNOME_INCLUDEDIR=\"-I$(includedir)\"" >> $(prefix)/lib/gnomeConf.sh