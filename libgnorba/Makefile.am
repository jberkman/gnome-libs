bin_PROGRAMS = loadshlib

lib_LTLIBRARIES = libgnorba.la

LINK = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) -o $@

libgnorbaincludedir = $(includedir)/libgnorba

INCLUDES = -I.. -I$(srcdir)/.. -I$(includedir) $(SUPPORTINCS) \
        $(GNOME_INCLUDES)  \
	$(ORBIT_CFLAGS) \
	$(GLIB_INCLUDES) \
	-DGNOMEBINDIR=$(bindir) \
	-DGNOMESYSCONFDIR=\"$(sysconfdir)\"

BUILT_SOURCES=\
	$(name_IDL_SOURCES)

name_IDL_SOURCES=\
	name-service.h name-service-skels.c name-service-stubs.c name-service-common.c

libgnorba_la_SOURCES = \
	name-service-skels.c name-service-stubs.c name-service-common.c \
	orbitgtk.c goad.c gnome-plugins.c

libgnorbainclude_HEADERS = \
	name-service.h \
	gnorba.h

libgnorba_la_LDFLAGS = -version-info 26:0:26 -rpath $(libdir)
libgnorba_la_LIBADD = $(ORBIT_LIBS) $(GLIB_LIBS) -lm

$(name_IDL_SOURCES): $(top_srcdir)/idl/name-service.idl
	orbit-idl $(top_srcdir)/idl/name-service.idl

loadshlib_SOURCES = loadshlib.c

loadshlib_LDADD = \
	$(ORBIT_LIBS) \
        $(top_builddir)/libgnome/libgnome.la		\
        $(top_builddir)/libgnomeui/libgnomeui.la	\
        @LIBSUPPORT@					\
        $(LIBGNOME_LIBS) $(LIBGNOMEUI_LIBS)		\
	$(top_builddir)/libgnorba/libgnorba.la		\
	$(INTLLIBS)

CLEANFILES = $(name_IDL_SOURCES)
