Utilitiesdir = $(datadir)/apps/Utilities
HELLO_HELP_DIR = $(datadir)/gnome-hello

# DEFS += -ggdb -Wall -DDEBUG

#INCLUDES = 			\
#	-I../.. 		\
#	-I$(srcdir)/../.. 	\
#	-I$(includedir)		\
#	-DHAS_GNOME=1		\
#	-DHELP_PATH=\"$(GTT_HELP_DIR)\" \
#	-DLOCALEDIR=\"$(datadir)/locale\" \
#	$(X_CFLAGS)

bin_PROGRAMS = gnome-hello

gnome_hello_SOURCES = gnome-hello.c

gnome_hello_LDADD = \
	$(top_builddir)/libgnomeui/libgnomeui.la 	\
	$(top_builddir)/libgnome/libgnome.la		\
	$(top_builddir)/intl/libintl.a			\
	$(top_builddir)/gtk-xmhtml/libgtkxmhtml.la	\
	$(XPM_LIBS)					\
	$(GTK_LIBS)					\
	$(DL_LIB)					\
        $(X_LIBS) $(x_libs) $(INTLLIBS)

htmldir = $(prefix)/html
html_DATA = gnome-hello.html

gnome-hello.html: gnome-hello.sgml
	db2html $<

gnome-hello.dvi: gnome-hello.sgml
	db2dvi $<

gnome-hello.ps: gnome-hello.dvi
	dvips -o $@ $<

# automake does not yet dig .sgml files, so for now the DocBook
# files must be extra_dist.
EXTRA_DIST = gnome-hello.sgml

#GTT_HELP_FILES = \
#	index.html		\
#	ch-dialogs.html		\
#	ch-main.html		\
#	ch-intro.html		\
#	ch-working.html
