bin_PROGRAMS = loadshlib goad-browser gnome-name-service

lib_LTLIBRARIES = libgnorba.la libgnorbagtk.la

LINK = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) -o $@

libgnorbaincludedir = $(includedir)/libgnorba

EXTRA_DIST = yes.xpm no.xpm

INCLUDES = 						\
	-I.. 						\
	-I$(srcdir)/..					\
	$(SUPPORTINCS) 					\
	-I$(includedir) 				\
        $(GNOME_INCLUDES)  				\
	$(ORBIT_CFLAGS) 				\
	$(GLIB_INCLUDES) 				\
	-DGNOMEBINDIR=$(bindir) 			\
	-DGNOMESYSCONFDIR=\"$(sysconfdir)\" 		\
	-DVERSION=\""$(VERSION)"\"

gnorba_built = 						\
	gnome-factory.h 				\
	gnome-factory-skels.c 				\
	gnome-factory-stubs.c 				\
	gnome-factory-common.c

ORBIT_IDL=orbit-idl

$(gnorba_built): $(top_srcdir)/idl/gnome-factory.idl
	$(ORBIT_IDL) $<

libgnorba_la_SOURCES = 					\
	$(gnorba_built) 				\
	goad.c 						\
	orbitgtk.c					\
	orbitgnome.c					\
	orbitns.c

libgnorbagtk_la_SOURCES = 				\
	$(gnorba_built) 				\
	orbitgtk.c

libgnorbainclude_HEADERS = 				\
	gnorba.h 					\
	gnome-factory.h

libgnorba_la_LDFLAGS = -version-info 28:0:1 -rpath $(libdir)
libgnorba_la_LIBADD = $(ORBIT_LIBS) $(GLIB_LIBS) -lm

loadshlib_SOURCES = loadshlib.c

loadshlib_LDADD = 					\
	$(top_builddir)/libgnorba/libgnorba.la		\
	$(ORBIT_LIBS) 					\
        $(top_builddir)/libgnome/libgnome.la		\
        $(top_builddir)/libgnomeui/libgnomeui.la	\
	$(top_builddir)/libart_lgpl/libart_lgpl.la	\
        @LIBSUPPORT@					\
        $(LIBGNOMEUI_LIBS) $(LIBGNOME_LIBS)		\
	$(INTLLIBS)

goad_browser_SOURCES = 	\
	goad-browser.c

goad_browser_LDADD = 					\
	$(top_builddir)/libgnorba/libgnorba.la		\
	$(ORBIT_LIBS) 					\
        $(top_builddir)/libgnomeui/libgnomeui.la	\
        $(top_builddir)/libgnome/libgnome.la		\
	$(top_builddir)/libart_lgpl/libart_lgpl.la	\
        @LIBSUPPORT@					\
        $(LIBGNOMEUI_LIBS) $(LIBGNOME_LIBS) 		\
	$(INTLLIBS)

gnome_name_service_SOURCES =				\
	gnome-name-server.c

gnome_name_service_LDADD = 				\
	$(top_builddir)/libgnorba/libgnorbagtk.la	\
	$(ORBIT_LIBS) 					\
	$(X_LIBS) -lX11					\
	-lname-server					\
	$(INTLLIBS)

gbpixmapdir = $(datadir)/pixmaps
gbpixmap_DATA = yes.xpm no.xpm

install-data-hook:
	ln -sf goad-browser $(DESTDIR)$(bindir)/new-object
