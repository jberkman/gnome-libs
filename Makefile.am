built_SUBDIRS = libgnome libgnomeui gtk-xmhtml gnome-hello libgtktty

SUBDIRS = po intl support $(built_SUBDIRS)

DISTCLEANFILES = stamp-v

include_HEADERS = gnome.h

EXTRA_DIST = HACKING

release:
	$(MAKE) disk distdir=$(PACKAGE)$(VERSION)

confdir=$(libdir)
conf_DATA = gnomeConf.sh
