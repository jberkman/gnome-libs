INCLUDES = -I.. -I$(srcdir)/.. -I$(includedir) @SUPPORTINCS@ \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DLOCALEDIR=\"$(datadir)/locale\"

noinst_PROGRAMS = testgnome

testgnome_SOURCES = 	\
	canvas.c	\
	testgnome.c	\
	testgnome.h

EXTRA_DIST = \
	bomb.xpm

testgnome_LDADD = \
	$(top_builddir)/libgnomeui/libgnomeui.la 	\
	$(top_builddir)/libgnome/libgnome.la		\
	@LIBSUPPORT@					\
	$(LIBGTKXMHTML_LIBS) $(LIBGNOME_LIBS) $(LIBGNOMEUI_LIBS) $(INTLLIBS)
