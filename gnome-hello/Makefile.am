#Utilitiesdir = $(datadir)/apps/Utilities

INCLUDES = -I../.. -I$(srcdir)/../.. -I$(includedir) \
	 -DLOCALEDIR=\"$(datadir)/locale\"

bin_PROGRAMS = gnome-hello

gnome_hello_SOURCES = gnome-hello.c

gnome_hello_LDADD = \
	$(top_builddir)/libgnomeui/libgnomeui.la 	\
	$(top_builddir)/libgnome/libgnome.la		\
	$(top_builddir)/gtk-xmhtml/libgtkxmhtml.la	\
	$(XPM_LIBS)					\
	$(GTK_LIBS)					\
	$(DL_LIB)					\
        $(X_LIBS) $(x_libs) $(INTLLIBS)			\
	$(top_builddir)/support/libsupport.a

htmldir = $(prefix)/html
html_DATA = gnome-hello.html

gnome-hello.html: gnome-hello.sgml
	-db2html $<

gnome-hello.dvi: gnome-hello.sgml
	-db2dvi $<

gnome-hello.ps: gnome-hello.dvi
	-dvips -o $@ $<

# automake does not yet dig .sgml files, so for now the DocBook
# files must be extra_dist.  Also, distribute the `.html' file
# so that people without DocBook can still use the docs.
EXTRA_DIST = gnome-hello.sgml gnome-hello.html
