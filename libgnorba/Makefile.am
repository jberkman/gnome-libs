bin_PROGRAMS = loadshlib #goad-browser

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

libgnorba_la_SOURCES = \
	orbitgtk.c goad.c gnome-plugins.c

libgnorbainclude_HEADERS = \
	gnorba.h

libgnorba_la_LDFLAGS = -version-info 26:0:26 -rpath $(libdir)
libgnorba_la_LIBADD = $(ORBIT_LIBS) $(GLIB_LIBS) -lm

loadshlib_SOURCES = loadshlib.c

loadshlib_LDADD = \
	$(ORBIT_LIBS) \
        $(top_builddir)/libgnome/libgnome.la		\
        $(top_builddir)/libgnomeui/libgnomeui.la	\
        @LIBSUPPORT@					\
        $(LIBGNOME_LIBS) $(LIBGNOMEUI_LIBS)		\
	$(top_builddir)/libgnorba/libgnorba.la		\
	$(INTLLIBS)

#goad_browser_SOURCES = goad-browser.c

#goad_browser_LDADD = \
#	$(ORBIT_LIBS) \
#        $(top_builddir)/libgnome/libgnome.la		\
#        $(top_builddir)/libgnomeui/libgnomeui.la	\
#        @LIBSUPPORT@					\
#        $(LIBGNOME_LIBS) $(LIBGNOMEUI_LIBS)		\
#	$(top_builddir)/libgnorba/libgnorba.la		\
#	$(INTLLIBS)

