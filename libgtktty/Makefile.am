
LINK     = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) $(X_LIBS) -o $@

libgemvtincludedir = $(includedir)/gemvt

INCLUDES = 			\
	-I../.. 		\
	-I$(srcdir)/../.. 	\
	-I$(includedir)		\
	$(X_CFLAGS)

lib_LTLIBRARIES = libgemvt.la

bin_PROGRAMS = gemvt

libgemvt_la_SOURCES = 	\
	gtkled.c 	\
	gtkterm.c 	\
	gtkvtemu.c 	\
	gtktty.c

libgemvtinclude_HEADERS =\
	gtkled.h 	\
	gtkterm.h 	\
	gtkvtemu.h 	\
	gtktty.h 	\
	libgemvt.h

EXTRA_DIST = \
	TODO \
	gtkterm_internal.c \
	gtkttyos.c 	\
	gtkvt102.c 	\
	gemvt.xpm \
	gem-r.xpm \
	gem-g.xpm \
	gem-b.xpm \
	turkey.vt

gemvt_SOURCES = \
	main.c

gemvt_LDADD = $(X_LIBS) $(top_builddir)/libgemvt.la

libgemvt_la_LDFLAGS = -version-info 0:0:0 -rpath $(libdir)