built_SUBDIRS = libgnome libgnomeui gtk-xmhtml gnome-hello libgtktty

SUBDIRS = po intl support macros $(built_SUBDIRS) 

DISTCLEANFILES = stamp-v

include_HEADERS = gnome.h

EXTRA_DIST = HACKING

release:
	$(MAKE) dist distdir=$(PACKAGE)$(VERSION)

confdir=$(libdir)
conf_DATA = gnomeConf.sh

## to automatically rebuild aclocal.m4 if any of the macros in
## `macros/' change
@MAINT@include macros/macros.dep
@MAINT@macros/macros.dep: macros/Makefile.am
@MAINT@	cd macros && $(MAKE) macros.dep

## We create gnomeConf.sh here and not from configure because we want
## to get the paths expanded correctly.  Macros like srcdir are given
## the value NONE in configure if the user doesn't specify them (this
## is an autoconf feature, not a bug).
gnomeConf.sh: gnomeConf.sh.in Makefile
## Use sed and then mv to avoid problems if the user interrupts.
	sed -e 's,\@GNOME_LIBDIR\@,$(GNOME_LIBDIR),g' \
	    -e 's,\@GNOME_INCLUDEDIR\@,$(GNOME_INCLUDEDIR),g' \
	    -e 's,\@GNOME_LIBS\@,$(GNOME_LIBS),g' \
	    -e 's,\@GNOMEUI_LIBS\@,$(GNOMEUI_LIBS),g' \
	    -e 's,\@GTKXMHTML_LIBS\@,$(GTKXMHTML_LIBS),g' \
	      < $(srcdir)/gnomeConf.sh.in > gnomeConf.tmp \
	  && mv gnomeConf.tmp gnomeConf.sh
