## Process this file with automake to produce Makefile.in

INCLUDES = -I.. -I$(srcdir)/.. -I$(includedir) $(GDK_IMLIB_CFLAGS) $(SUPPORTINCS) \
	-I$(top_srcdir)/intl -I$(top_builddir)/intl \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DLOCALEDIR=\"$(datadir)/locale\"

noinst_PROGRAMS = testgnome

testgnome_SOURCES =		\
	canvas.c		\
	canvas-primitives.c	\
	canvas-arrowhead.c	\
	canvas-fifteen.c	\
	testgnome.c		\
	testgnome.h

EXTRA_DIST = 		\
	bomb.xpm	\
	flower.png	\
	toroid.png

testgnome_LDADD = \
	$(top_builddir)/libgnomeui/libgnomeui.la 	\
	$(top_builddir)/libgnome/libgnome.la		\
	@LIBSUPPORT@					\
	$(LIBGTKXMHTML_LIBS) $(LIBGNOME_LIBS) $(LIBGNOMEUI_LIBS) $(INTLLIBS)
