lib_LTLIBRARIES = libgnorba.la

LINK = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) -o $@

libgnorbaincludedir = $(includedir)/libgnorba

INCLUDES = -I.. -I$(srcdir)/.. -I$(includedir) $(SUPPORTINCS) \
        $(GNOME_INCLUDES)  \
	$(ORBIT_INCLUDES) \
	$(GLIB_INCLUDES) \
	-DGNOMEBINDIR=$(bindir)

BUILT_SOURCES=\
	$(name_IDL_SOURCES)

name_IDL_SOURCES=\
	name-service.h name-service-skels.c name-service-stubs.c name-service-common.c

libgnorba_la_SOURCES = \
	name-service-skels.c name-service-stubs.c name-service-common.c \
	orbitgtk.c

libgnorbainclude_HEADERS = \
	name-service.h

libgnorba_la_LDFLAGS = -version-info 26:0:26 -rpath $(libdir)
libgnorba_la_LIBADD = $(ORBIT_LIBS) $(GLIB_LIBS) -lm

$(name_IDL_SOURCES): $(top_srcdir)/idl/name-service.idl
	orbit-idl $(top_srcdir)/idl/name-service.idl
