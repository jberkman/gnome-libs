## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir)/gdk-pixbuf -I.. -I$(srcdir)/.. \
	$(GNOMESUPPORT_CFLAGS)				\
	-I$(top_srcdir)/intl -I$(top_builddir)/intl \
	$(GLIB_CFLAGS) $(GTK_CFLAGS)			\
	$(OAF_CFLAGS)					\
	$(LIBGLADE_CFLAGS)				\
	$(LIBART_CFLAGS)				\
	$(ORBIT_CFLAGS)					\
	$(GCONF_CFLAGS)					\
	-DGNOMELOCALEDIR=\""$(gnomelocaledir)"\" \
	-DLOCALEDIR=\""$(gnomelocaledir)"\"

noinst_PROGRAMS = test-gnome

test_gnome_SOURCES =		\
	canvas.c		\
	canvas-primitives.c	\
	canvas-arrowhead.c	\
	canvas-features.c	\
	canvas-fifteen.c	\
	testgnome.c		\
	testgnome.h

EXTRA_DIST = 		\
	bomb.xpm	\
	flower.png	\
	toroid.png	\
	widget-help-map.txt

test_gnome_LDADD = \
	$(top_builddir)/libgnomeui/libgnomeui-$(GNOME_INTERFACE_VERSION).la 	\
	$(top_builddir)/libgnome/libgnome-$(GNOME_INTERFACE_VERSION).la		\
	$(LIBGNOME_LIBS) $(LIBGNOMEUI_LIBS) @LIBART_LIBS@ $(INTLLIBS)

# the program has to be run from $srcdir to find its image files
test: test-gnome
	blddir=`pwd`; top_blddir=`cd $(top_builddir) && pwd`; cd $(srcdir); \
	$(SHELL) $$top_blddir/libtool --mode=execute $$blddir/test-gnome

test-debug: test-gnome
	blddir=`pwd`; top_blddir=`cd $(top_builddir) && pwd`; cd $(srcdir); \
	$(SHELL) $$top_blddir/libtool --mode=execute gdb $$blddir/test-gnome

