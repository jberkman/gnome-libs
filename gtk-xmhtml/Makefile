# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.2c from Makefile.am

# Copyright (C) 1994, 1995, 1996, 1997 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.


SHELL = /bin/sh

srcdir = .
top_srcdir = ..
prefix = /da/share/gnome-install
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

pkgdatadir = $(datadir)/gnome
pkglibdir = $(libdir)/gnome
pkgincludedir = $(includedir)/gnome

top_builddir = ..

ACLOCAL = aclocal
AUTOCONF = autoconf
AUTOMAKE = automake
AUTOHEADER = autoheader

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
transform = s,x,x,

NORMAL_INSTALL = true
PRE_INSTALL = true
POST_INSTALL = true
NORMAL_UNINSTALL = true
PRE_UNINSTALL = true
POST_UNINSTALL = true
host_alias = alpha-unknown-linux-gnu
host_triplet = alpha-unknown-linux-gnu
CATALOGS = 
CATOBJEXT = .mo
CC = gcc
CFLAGS = -Wall -O2 -g
CPPFLAGS = 
CXX = c++
DATADIRNAME = lib
DL_LIB = -ldl
ELCFILES = @ELCFILES@
EMACS = @EMACS@
GENCAT = 
GMOFILES = 
GMSGFMT = /usr/bin/msgfmt
GTK_LIBS = -lgtk -lgdk -lglib -lm
GUILE_LIBS = -lguile
INSTOBJEXT = .mo
INTLDEPS = 
INTLLIBS = 
INTLOBJS = 
INTLSUB = intl
JPEG_LIBS = -ljpeg
LD = /usr/bin/ld
LDFLAGS = 
LEX = flex
LIBTOOL = $(top_builddir)/libtool
LISPDIR = @LISPDIR@
LN_S = ln -s
MAKEINFO = makeinfo
MSGFMT = /usr/bin/msgfmt
NM = /usr/bin/nm -B
OBJC = gcc
OSOCKADDR_CFLAGS = 
PACKAGE = gnome
PNG_LIBS = 
POFILES = 
POSUB = po
PTHREAD_LIB = -lpthread
QTTHREADS_LIB = 
RANLIB = ranlib
READLINE_LIB = 
TERMCAP_LIB = 
VERSION = 0.11
XF86MISC_LIBS = -lXxf86misc
XPM_LIBS = -lXpm
YACC = 
Z_LIBS = -lz
x_libs = -lX11  -lXext -lSM -lICE

libgtkxmhtmlincludedir = $(includedir)/gtk-xmhtml

INCLUDES = -I.. -I$(srcdir)/.. -I$(includedir) -DWITH_GTK -DDEBUG -DGNOME_LIB

lib_LTLIBRARIES = libgtkxmhtml.la

libgtkxmhtml_la_SOURCES =	\
	XmHTML.c		\
	gtk-error.c		\
	gtk-forms.c		\
	frames.c		\
	parse.c			\
	StringUtil.c		\
	fonts.c			\
	paint.c			\
	map.c			\
	callbacks.c		\
	format.c		\
	plc.c			\
	readXPM.c		\
	readJPEG.c		\
	readJPEGplc.c		\
	readBitmap.c		\
	readGIF.c		\
	LZWStream.c		\
	LZWStream.h		\
	readGIFplc.c		\
	readFLG.c		\
	readPNG.c		\
	colors.c		\
	images.c		\
	quantize.c		\
	layout.c		\
	events.c		\
	debug.c			\
	XmHTML-motif.h		\
	XmHTMLfuncs.h		\
	debug.h			\
	escapes.h		\
	motif.h			\
	plc.h

libgtkxmhtmlinclude_HEADERS =	\
	miniparse.h		\
	HTML.h			\
	XmHTML.h		\
	XmHTMLI.h		\
	XmHTMLP.h		\
	gtk-xmhtml-p.h		\
	gtk-xmhtml.h		\
	toolkit.h

libgtkxmhtml_la_LDFLAGS = -version-info 0:0:0 -rpath $(libdir)
libgtkxmhtml_la_LIBADD = $(GTK_LIBS) $(X_LIBS) $(x_libs)

bin_PROGRAMS = htmltest

htmltest_SOURCES = test.c
htmltest_LDADD   = $(top_builddir)/gtk-xmhtml/libgtkxmhtml.la \
	$(GTK_LIBS) $(X_LIBS) $(x_libs) $(Z_LIBS) $(JPEG_LIBS) $(PNG_LIBS) $(XPM_LIBS)

EXTRA_DIST = gtk-xmhtml.c XmHTML-motif.c 
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../config.h
CONFIG_CLEAN_FILES = 
LTLIBRARIES =  $(lib_LTLIBRARIES)


DEFS = -DHAVE_CONFIG_H -I. -I$(srcdir) -I..
LIBS = 
X_CFLAGS =  -I/usr/X11R6/include
X_LIBS =  -L/usr/X11R6/lib
X_EXTRA_LIBS = 
X_PRE_LIBS =  -lSM -lICE
libgtkxmhtml_la_DEPENDENCIES = 
libgtkxmhtml_la_OBJECTS =  XmHTML.lo gtk-error.lo gtk-forms.lo frames.lo \
parse.lo StringUtil.lo fonts.lo paint.lo map.lo callbacks.lo format.lo \
plc.lo readXPM.lo readJPEG.lo readJPEGplc.lo readBitmap.lo readGIF.lo \
LZWStream.lo readGIFplc.lo readFLG.lo readPNG.lo colors.lo images.lo \
quantize.lo layout.lo events.lo debug.lo
PROGRAMS =  $(bin_PROGRAMS)

htmltest_OBJECTS =  test.o
htmltest_DEPENDENCIES =  $(top_builddir)/gtk-xmhtml/libgtkxmhtml.la
htmltest_LDFLAGS = 
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
LTCOMPILE = $(LIBTOOL) --mode=compile $(CC) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
LINK = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) -o $@
HEADERS =  $(libgtkxmhtmlinclude_HEADERS)

DIST_COMMON =  ChangeLog Makefile.am Makefile.in TODO


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP = --best
DEP_FILES =  .deps/LZWStream.P .deps/StringUtil.P .deps/XmHTML.P \
.deps/callbacks.P .deps/colors.P .deps/debug.P .deps/events.P \
.deps/fonts.P .deps/format.P .deps/frames.P .deps/gtk-error.P \
.deps/gtk-forms.P .deps/images.P .deps/layout.P .deps/map.P \
.deps/paint.P .deps/parse.P .deps/plc.P .deps/quantize.P \
.deps/readBitmap.P .deps/readFLG.P .deps/readGIF.P .deps/readGIFplc.P \
.deps/readJPEG.P .deps/readJPEGplc.P .deps/readPNG.P .deps/readXPM.P \
.deps/test.P
SOURCES = $(libgtkxmhtml_la_SOURCES) $(htmltest_SOURCES)
OBJECTS = $(libgtkxmhtml_la_OBJECTS) $(htmltest_OBJECTS)

default: all

.SUFFIXES:
.SUFFIXES: .c .lo .o
$(srcdir)/Makefile.in: Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --gnu gtk-xmhtml/Makefile

Makefile: $(srcdir)/Makefile.in $(top_builddir)/config.status $(BUILT_SOURCES)
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status


mostlyclean-libLTLIBRARIES:

clean-libLTLIBRARIES:
	-test -z "$(lib_LTLIBRARIES)" || rm -f $(lib_LTLIBRARIES)

distclean-libLTLIBRARIES:

maintainer-clean-libLTLIBRARIES:

install-libLTLIBRARIES: $(lib_LTLIBRARIES)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(libdir)
	@list='$(lib_LTLIBRARIES)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo "$(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p $(libdir)/$$p"; \
	    $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p $(libdir)/$$p; \
	  else :; fi; \
	done

uninstall-libLTLIBRARIES:
	$(NORMAL_UNINSTALL)
	list='$(lib_LTLIBRARIES)'; for p in $$list; do \
	  $(LIBTOOL)  --mode=uninstall rm -f $(libdir)/$$p; \
	done

.c.o:
	$(COMPILE) -c $<

mostlyclean-compile:
	-rm -f *.o core

clean-compile:

distclean-compile:
	-rm -f *.tab.c

maintainer-clean-compile:

.c.lo:
	$(LIBTOOL) --mode=compile $(COMPILE) -c $<

mostlyclean-libtool:
	-rm -f *.lo

clean-libtool:
	-rm -rf .libs

distclean-libtool:

maintainer-clean-libtool:

libgtkxmhtml.la: $(libgtkxmhtml_la_OBJECTS) $(libgtkxmhtml_la_DEPENDENCIES)
	$(LINK) -rpath $(libdir) $(libgtkxmhtml_la_LDFLAGS) $(libgtkxmhtml_la_OBJECTS) $(libgtkxmhtml_la_LIBADD) $(LIBS)

mostlyclean-binPROGRAMS:

clean-binPROGRAMS:
	-test -z "$(bin_PROGRAMS)" || rm -f $(bin_PROGRAMS)

distclean-binPROGRAMS:

maintainer-clean-binPROGRAMS:

install-binPROGRAMS: $(bin_PROGRAMS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(bindir)
	@list='$(bin_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo " $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p $(bindir)/`echo $$p|sed '$(transform)'`"; \
	    $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p $(bindir)/`echo $$p|sed '$(transform)'`; \
	  else :; fi; \
	done

uninstall-binPROGRAMS:
	$(NORMAL_UNINSTALL)
	list='$(bin_PROGRAMS)'; for p in $$list; do \
	  rm -f $(bindir)/`echo $$p|sed '$(transform)'`; \
	done

htmltest: $(htmltest_OBJECTS) $(htmltest_DEPENDENCIES)
	@rm -f htmltest
	$(LINK) $(htmltest_LDFLAGS) $(htmltest_OBJECTS) $(htmltest_LDADD) $(LIBS)

install-libgtkxmhtmlincludeHEADERS: $(libgtkxmhtmlinclude_HEADERS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(libgtkxmhtmlincludedir)
	@list='$(libgtkxmhtmlinclude_HEADERS)'; for p in $$list; do \
	  echo " $(INSTALL_DATA) $(srcdir)/$$p $(libgtkxmhtmlincludedir)/$$p"; \
	  $(INSTALL_DATA) $(srcdir)/$$p $(libgtkxmhtmlincludedir)/$$p; \
	done

uninstall-libgtkxmhtmlincludeHEADERS:
	$(NORMAL_UNINSTALL)
	list='$(libgtkxmhtmlinclude_HEADERS)'; for p in $$list; do \
	  rm -f $(libgtkxmhtmlincludedir)/$$p; \
	done

tags: TAGS

ID: $(HEADERS) $(SOURCES)
	here=`pwd` && cd $(srcdir) && mkid -f$$here/ID $(SOURCES) $(HEADERS)

TAGS:  $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES)
	tags=; \
	here=`pwd`; \
	test -z "$(ETAGS_ARGS)$(SOURCES)$(HEADERS)$$tags" \
	  || (cd $(srcdir) && etags $(ETAGS_ARGS) $$tags  $(SOURCES) $(HEADERS) -o $$here/TAGS)

mostlyclean-tags:

clean-tags:

distclean-tags:
	-rm -f TAGS ID

maintainer-clean-tags:

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = gtk-xmhtml

distdir: $(DISTFILES)
	here=`cd $(top_builddir) && pwd`; \
	top_distdir=`cd $(top_distdir) && pwd`; \
	distdir=`cd $(distdir) && pwd`; \
	cd $(top_srcdir) \
	  && $(AUTOMAKE) --include-deps --build-dir=$$here --srcdir-name=$(top_srcdir) --output-dir=$$top_distdir --gnu gtk-xmhtml/Makefile
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  test -f $(distdir)/$$file \
	  || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	  || cp -p $$d/$$file $(distdir)/$$file; \
	done
	$(MAKE) top_distdir="$(top_distdir)" distdir="$(distdir)" dist-hook

MKDEP = $(CC) -M $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)

DEPS_MAGIC := $(shell mkdir .deps > /dev/null 2>&1 || :)
-include .deps/.P
.deps/.P: $(BUILT_SOURCES)
	echo > $@

-include $(DEP_FILES)

mostlyclean-depend:

clean-depend:

distclean-depend:

maintainer-clean-depend:
	-rm -rf .deps

.deps/%.P: %.c
	@echo "Computing dependencies for $<..."
	@o='o'; \
	test -n "$o" && o='$$o'; \
	$(MKDEP) $< >$@.tmp \
	  && sed "s,^\(.*\)\.o:,\1.$$o \1.l$$o $@:," < $@.tmp > $@ \
	  && rm -f $@.tmp
info:
dvi:
check: all
	$(MAKE)
installcheck:
install-exec: install-libLTLIBRARIES install-binPROGRAMS
	@$(NORMAL_INSTALL)

install-data: install-libgtkxmhtmlincludeHEADERS
	@$(NORMAL_INSTALL)

install: install-exec install-data all
	@:

uninstall: uninstall-libLTLIBRARIES uninstall-binPROGRAMS uninstall-libgtkxmhtmlincludeHEADERS

all: Makefile $(LTLIBRARIES) $(PROGRAMS) $(HEADERS)

install-strip:
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' INSTALL_SCRIPT='$(INSTALL_PROGRAM)' install
installdirs:
	$(mkinstalldirs)  $(libdir) $(bindir) $(libgtkxmhtmlincludedir)


mostlyclean-generic:
	-test -z "$(MOSTLYCLEANFILES)" || rm -f $(MOSTLYCLEANFILES)

clean-generic:
	-test -z "$(CLEANFILES)" || rm -f $(CLEANFILES)

distclean-generic:
	-rm -f Makefile $(DISTCLEANFILES)
	-rm -f config.cache config.log stamp-h stamp-h[0-9]*
	-test -z "$(CONFIG_CLEAN_FILES)" || rm -f $(CONFIG_CLEAN_FILES)

maintainer-clean-generic:
	-test -z "$(MAINTAINERCLEANFILES)" || rm -f $(MAINTAINERCLEANFILES)
	-test -z "$(BUILT_SOURCES)" || rm -f $(BUILT_SOURCES)
mostlyclean:  mostlyclean-libLTLIBRARIES mostlyclean-compile \
		mostlyclean-libtool mostlyclean-binPROGRAMS \
		mostlyclean-tags mostlyclean-depend mostlyclean-generic

clean:  clean-libLTLIBRARIES clean-compile clean-libtool \
		clean-binPROGRAMS clean-tags clean-depend clean-generic \
		mostlyclean

distclean:  distclean-libLTLIBRARIES distclean-compile distclean-libtool \
		distclean-binPROGRAMS distclean-tags distclean-depend \
		distclean-generic clean
	-rm -f config.status
	-rm -f libtool

maintainer-clean:  maintainer-clean-libLTLIBRARIES \
		maintainer-clean-compile maintainer-clean-libtool \
		maintainer-clean-binPROGRAMS maintainer-clean-tags \
		maintainer-clean-depend maintainer-clean-generic \
		distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

.PHONY: default mostlyclean-libLTLIBRARIES distclean-libLTLIBRARIES \
clean-libLTLIBRARIES maintainer-clean-libLTLIBRARIES \
uninstall-libLTLIBRARIES install-libLTLIBRARIES mostlyclean-compile \
distclean-compile clean-compile maintainer-clean-compile \
mostlyclean-libtool distclean-libtool clean-libtool \
maintainer-clean-libtool mostlyclean-binPROGRAMS distclean-binPROGRAMS \
clean-binPROGRAMS maintainer-clean-binPROGRAMS uninstall-binPROGRAMS \
install-binPROGRAMS uninstall-libgtkxmhtmlincludeHEADERS \
install-libgtkxmhtmlincludeHEADERS tags mostlyclean-tags distclean-tags \
clean-tags maintainer-clean-tags distdir mostlyclean-depend \
distclean-depend clean-depend maintainer-clean-depend info dvi \
installcheck install-exec install-data install uninstall all \
installdirs mostlyclean-generic distclean-generic clean-generic \
maintainer-clean-generic clean mostlyclean distclean maintainer-clean


dist-hook:
	mkdir $(distdir)/bitmaps
	cp bitmaps/*.xpm bitmaps/*.xbm $(distdir)/bitmaps

gtk-xmhtml:
	echo

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
