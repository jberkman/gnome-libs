LINK     = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) $(GTK_LIBS) -o $@
INCLUDE_always = -I$(includedir) $(GTK_CFLAGS)

INCLUDES = -I.. -I$(srcdir)/../.. $(INCLUDE_always)

lib_LTLIBRARIES = libgtktty.la
libgtkttyincludedir = $(includedir)/gtktty
libgtkttyinclude_HEADERS = $(libgtktty_h_files)
libgtktty_la_SOURCES = $(libgtktty_c_files)
libgtktty_la_LDFLAGS = -version-info $(LIBGTKTTY_MAJOR):$(LIBGTKTTY_REVISION):$(LIBGTKTTY_AGE)


libgtktty_h_files = \
	gtkled.h        \
	gtkterm.h       \
	gtkvtemu.h      \
	gtktty.h	\
	libgtktty.h

libgtktty_h_files_extra = \
	gtkvtemui.h	\
	gtkvt102.h

libgtktty_c_files =   \
	gtkled.c        \
	gtkterm.c       \
	gtkvtemu.c      \
	gtkvt102.c	\
	gtktty.c	\
	libgtktty.c

libgtktty_c_files_extra = \
	gtkterm_internal.c \
	gtkttyos.c \
	gtkvt102c2f.c

EXTRA_DIST = \
	TODO \
	$(libgtktty_h_files) \
	$(libgtktty_h_files_extra) \
	$(libgtktty_c_files) \
	$(libgtktty_c_files_extra) \
	turkey.vt
