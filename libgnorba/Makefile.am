bin_PROGRAMS = loadshlib goad-browser

lib_LTLIBRARIES = libgnorba.la

LINK = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) -o $@

libgnorbaincludedir = $(includedir)/libgnorba

INCLUDES = -I.. -I$(srcdir)/.. $(SUPPORTINCS) -I$(includedir) \
        $(GNOME_INCLUDES)  \
	$(ORBIT_CFLAGS) \
	$(GLIB_INCLUDES) \
	-DGNOMEBINDIR=$(bindir) \
	-DGNOMESYSCONFDIR=\"$(sysconfdir)\" \
	-DVERSION=\""$(VERSION)"\"

gnorba_built=gnome-factory.h gnome-factory-skels.c \
	gnome-factory-stubs.c gnome-factory-common.c

ORBIT_IDL=orbit-idl

$(gnorba_built): $(top_srcdir)/idl/gnome-factory.idl
	$(ORBIT_IDL) $<

libgnorba_la_SOURCES = \
	orbitgtk.c goad.c gnome-plugins.c \
	gnome-factory-skels.c gnome-factory-stubs.c \
	gnome-factory-common.c

libgnorbainclude_HEADERS = \
	gnorba.h gnome-factory.h

libgnorba_la_LDFLAGS = -version-info 26:0:26 -rpath $(libdir)
libgnorba_la_LIBADD = $(ORBIT_LIBS) $(GLIB_LIBS) -lm

loadshlib_SOURCES = loadshlib.c

loadshlib_LDADD = \
	$(ORBIT_LIBS) \
        $(top_builddir)/libgnome/libgnome.la		\
        $(top_builddir)/libgnomeui/libgnomeui.la	\
	$(top_builddir)/libart_lgpl/libart_lgpl.la	\
        @LIBSUPPORT@					\
        $(LIBGNOME_LIBS) $(LIBGNOMEUI_LIBS)		\
	$(top_builddir)/libgnorba/libgnorba.la		\
	$(INTLLIBS)

goad_browser_SOURCES = goad-browser.c

goad_browser_LDADD = \
	$(ORBIT_LIBS) \
        $(top_builddir)/libgnome/libgnome.la		\
        $(top_builddir)/libgnomeui/libgnomeui.la	\
	$(top_builddir)/libart_lgpl/libart_lgpl.la	\
        @LIBSUPPORT@					\
        $(LIBGNOME_LIBS) $(LIBGNOMEUI_LIBS)		\
	$(top_builddir)/libgnorba/libgnorba.la		\
	$(INTLLIBS)

gbpixmapdir = $(datadir)/pixmaps
gbpixmap_DATA = yes.xpm no.xpm
