bin_PROGRAMS = gemvt

LINK     = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) $(X_LIBS) -o $@
INCLUDE_always = -I$(includedir) $(X_CFLAGS)
GNOMELIBS = \
	../../libgnomeui/libgnomeui.la \
	../../libgnome/libgnome.la

if	BUILD_LIBGEMVT

INCLUDES = -I../.. -I$(srcdir)/../.. $(INCLUDE_always)
gemvt_SOURCES = $(gmevt_c_files) 
gemvt_LDADD = $(top_builddir)/libgemvt.la $(GNOMELIBS) $(GETOPT_OBJS) \
	      $(INTLLIBS)

lib_LTLIBRARIES = libgemvt.la
libgemvtincludedir = $(includedir)/gemvt
libgemvtinclude_HEADERS = $(libgemvt_h_files)
libgemvt_la_SOURCES = $(libgemvt_c_files)
libgemvt_la_LDFLAGS = -version-info $(LIBGEMVT_MAJOR):$(LIBGEMVT_REVISION):$(LIBGEMVT_AGE)

else	# ! BUILD_LIBGEMVT

INCLUDES = $(INCLUDE_always)
gemvt_SOURCES = $(libgemvt_c_files) $(gmevt_c_files) 
gemvt_LDADD = $(GETOPT_OBJS)

endif	# ! BUILD_LIBGEMVT

gemvt_DEPENDENCIES = $(GETOPT_OBJS)


gmevt_h_files = \
	gvtgui.h \
	gvtconf.h

gmevt_c_files = \
	gvtmain.c \
	gvtgui.c \
	gvtconf.c

libgemvt_h_files = \
	gtkled.h        \
	gtkterm.h       \
	gtkvtemu.h      \
	gtktty.h	\
	libgemvt.h

libgemvt_h_files_extra = \
	gtkvtemui.h	\
	gtkvt102.h

libgemvt_c_files =   \
	gtkled.c        \
	gtkterm.c       \
	gtkvtemu.c      \
	gtkvt102.c	\
	gtktty.c	\
	libgemvt.c

libgemvt_c_files_extra = \
	gtkterm_internal.c \
	gtkttyos.c \
	gtkvt102c2f.c

EXTRA_DIST = \
	TODO \
	$(gmevt_h_files) \
	$(gmevt_c_files) \
	$(libgemvt_h_files) \
	$(libgemvt_h_files_extra) \
	$(libgemvt_c_files) \
	$(libgemvt_c_files_extra) \
	getopt.h getopt.c getopt1.c \
	gemvt.xpm \
	gem-r.xpm \
	gem-g.xpm \
	gem-b.xpm \
	turkey.vt
