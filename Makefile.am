built_SUBDIRS = libgnome libgnomeui gtk-xmhtml gnome-hello libgtktty

SUBDIRS = po intl support macros $(built_SUBDIRS) 

DISTCLEANFILES = stamp-v

include_HEADERS = gnome.h

EXTRA_DIST = HACKING

release:
	$(MAKE) disk distdir=$(PACKAGE)$(VERSION)

confdir=$(libdir)
conf_DATA = gnomeConf.sh

## to automatically rebuild aclocal.m4 if any of the macros in `macros/' change
@MAINT@include macros/macros.dep
@MAINT@macros/macros.dep: macros/Makefile.am
@MAINT@	cd macros && $(MAKE) macros.dep
