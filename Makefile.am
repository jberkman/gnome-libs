## Process this file with automake to produce Makefile.in.

# Actually the earliest release that should be OK should be 1.2e, I think.
# Still, 1.3 is the first "released" automake that is OK.
AUTOMAKE_OPTIONS = 1.3

bin_SCRIPTS = gnome-config gnome-bug

if COMPILE_TEST_GNOME
test_gnome = demos test-gnome test-suite
else
test_gnome =
endif

if COMPILE_GNOME_COMPAT
libcompat_SUBDIRS = libcompat
else
libcompat_SUBDIRS =
endif


gnome_vfs_SUBDIRS=
gconf_SUBDIRS=

zvt_SUBDIRS=

SUBDIRS = \
	tools			\
	images			\
	libgnome		\
	libgnomeui		\
	$(libcompat_SUBDIRS)	\
	$(zvt_SUBDIRS)		\
	idl			\
	$(gconf_SUBDIRS)	\
	$(gnome_vfs_SUBDIRS)	\
	$(test_gnome)		\
	gnome-data man

if COMPILE_TEST_GNOME
compat_headers = gnome-compat-1.0.h
else
compat_headers =
endif

gnomeincludedir=$(includedir)/gnome/$(GNOME_INTERFACE_VERSION)
gnomeinclude_HEADERS = gnome.h $(compat_headers)

dist-hook: gnome-libs.spec
	cp gnome-libs.spec $(distdir)

release:
	$(MAKE) dist distdir=$(PACKAGE)$(VERSION)

## Put `exec' in the name because this should be installed by
## `install-exec', not `install-data'.
confexecdir=$(datadir)/gnome/conf/$(GNOME_INTERFACE_VERSION)
confexec_DATA=gnomeConf.sh

## We create gnomeConf.sh here and not from configure because we want
## to get the paths expanded correctly.  Macros like srcdir are given
## the value NONE in configure if the user doesn't specify them (this
## is an autoconf feature, not a bug).
gnomeConf.sh: gnomeConf.sh.in Makefile
## Use sed and then mv to avoid problems if the user interrupts.
	sed -e 's^\@GNOME_LIBDIR\@^$(GNOME_LIBDIR)^g' \
	    -e 's^\@GNOME_INCLUDEDIR\@^$(GNOME_INCLUDEDIR)^g' \
	    -e 's^\@GNOME_LIBS\@^$(GNOME_LIBS)^g' \
	    -e 's^\@GNOMEUI_LIBS\@^$(GNOMEUI_LIBS)^g' \
	    -e 's^\@GTKXMHTML_LIBS\@^$(GTKXMHTML_LIBS)^g' \
            -e 's^\@GTK_CFLAGS\@^$(GTK_CFLAGS)^g' \
	    -e 's^\@ZVT_LIBS\@^$(ZVT_LIBS)^g' \
	    -e 's^\@need_gnome_support\@^$(need_gnome_support)^g' \
	      < $(srcdir)/gnomeConf.sh.in > gnomeConf.tmp \
	  && mv gnomeConf.tmp gnomeConf.sh

compatConf.sh: compatConf.sh.in Makefile
## Use sed and then mv to avoid problems if the user interrupts.
	sed -e 's^\@GNOME_LIBDIR\@^$(GNOME_LIBDIR)^g' \
	    -e 's^\@GNOME_INCLUDEDIR\@^$(GNOME_INCLUDEDIR)^g' \
	    -e 's^\@GNOME_LIBS\@^$(GNOME_LIBS)^g' \
	    -e 's^\@GNOMEUI_LIBS\@^$(GNOMEUI_LIBS)^g' \
	    -e 's^\@GTKXMHTML_LIBS\@^$(GTKXMHTML_LIBS)^g' \
            -e 's^\@GTK_CFLAGS\@^$(GTK_CFLAGS)^g' \
	    -e 's^\@need_gnome_support\@^$(need_gnome_support)^g' \
	      < $(srcdir)/compatConf.sh.in > compatConf.tmp \
	  && mv compatConf.tmp compatConf.sh

CLEANFILES=gnomeConf.sh

EXTRA_DIST =	HACKING gnomeConf.sh.in gnome-bug.in gnome-config.in \
		gnome-libs.spec.in message-of-doom

