
INCLUDES = -I.. -I$(srcdir)/.. 	\
	-I$(includedir) 	\
	@SUPPORTINCS@ 		\
        @GTK_CFLAGS@  		\
	-I$(top_srcdir)/intl 	\
	-I$(top_srcdir)/support

lib_LTLIBRARIES = libzvt.la

libzvt_la_SOURCES =		\
	lists.c			\
	lists.h			\
	memory.c		\
	memory.h		\
	update.c		\
	vt.c			\
	vt.h			\
	vtx.h			\
	zvtterm.c		\

libzvtinclude_HEADERS =	\
	zvtterm.h

EXTRA_DIST = TODO BUGS README 

noinst_PROGRAMS = zterm

zterm_SOURCES	= zterm.c
zterm_LDADD	= 	\
	$(top_builddir)/zvt/libzvt.la	\
	$(GTK_LIBS) \
	@LIBSUPPORT@ $(INTLLIBS) -lutil

