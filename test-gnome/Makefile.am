## Process this file with automake to produce Makefile.in

INCLUDES = -I.. -I$(srcdir)/.. $(SUPPORTINCS) \
	-I$(top_srcdir)/intl -I$(top_builddir)/intl \
	-I$(includedir) $(GDK_IMLIB_CFLAGS) \
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
	toroid.png

test_gnome_LDADD = \
	$(top_builddir)/libgnomeui/libgnomeui.la 	\
	$(top_builddir)/libgnome/libgnome.la		\
	$(top_builddir)/libart_lgpl/libart_lgpl.la	\
	@LIBSUPPORT@					\
	$(LIBGTKXMHTML_LIBS) $(LIBGNOME_LIBS) $(LIBGNOMEUI_LIBS) $(INTLLIBS)
