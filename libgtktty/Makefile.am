
LINK     = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) $(X_LIBS) -o $@

INCLUDES = 			\
	-I../.. 		\
	-I$(srcdir)/../.. 	\
	-I$(includedir)		\
	$(X_CFLAGS)

bin_PROGRAMS = gemvt

.PHONY: .exe
.exe:
	echo libgemvt_la_LDFLAGS=$(libgemvt_la_LDFLAGS)
	
	
if	BUILD_LIBGEMVT

lib_LTLIBRARIES = libgemvt.la
libgemvtincludedir = $(includedir)/gemvt
libgemvtinclude_HEADERS = libgemvt.h $(libgemvt_h_files)
libgemvt_la_SOURCES = libgemvt.c $(libgemvt_c_files)
libgemvt_la_LDFLAGS = -version-info $(LIBGEMVT_MAJOR):$(LIBGEMVT_REVISION):$(LIBGEMVT_AGE)

gemvt_SOURCES = main.c
gemvt_LDADD = $(top_builddir)/libgemvt.la -lgnomeui -lgnome

else	# not BUILD_LIBGEMVT

gemvt_SOURCES = $(libgemvt_h_files) $(libgemvt_c_files) main.c
gemvt_LDADD =

endif	# not BUILD_LIBGEMVT


libgemvt_h_files =\
	gtkled.h        \
	gtkterm.h       \
	gtkvtemu.h      \
	gtktty.h
	libgemvt.h

libgemvt_c_files =   \
	gtkled.c        \
	gtkterm.c       \
	gtkvtemu.c      \
	gtktty.c

EXTRA_DIST = \
	TODO \
	gtkterm_internal.c \
	gtkvt102.c \
	gtkttyos.c \
	gemvt.xpm \
	gem-r.xpm \
	gem-g.xpm \
	gem-b.xpm \
	turkey.vt

##### blub = \
#	gtkled.h \
#	gtkled.c \
#	gtkterm.h \
#	gtkterm.c \
#	gtkvtemu.h \
#	gtkvtemu.c \
#	gtktty.h \
#	gtktty.c
