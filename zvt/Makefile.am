libzvtincludedir = $(includedir)/zvt

INCLUDES = -I.. -I$(srcdir)/.. 	\
	-I$(includedir) 	\
	@SUPPORTINCS@ 		\
        @GTK_CFLAGS@  		\
	-I$(top_srcdir)/intl 	\
	-I$(top_srcdir)/support

lib_LTLIBRARIES = libzvt.la

libzvt_la_SOURCES =		\
	forkpty.c		\
	lists.c			\
	memory.c		\
	memory.h		\
	update.c		\
	vt.c			\
	zvtterm.c

libzvtinclude_HEADERS =	\
	zvtterm.h	\
	vtx.h		\
	lists.h		\
	forkpty.h	\
	vt.h

EXTRA_DIST = TODO BUGS README 

noinst_PROGRAMS = zterm

zterm_SOURCES	= zterm.c
zterm_LDADD	= 	\
	$(top_builddir)/zvt/libzvt.la	\
	$(GTK_LIBS) \
	@LIBSUPPORT@ $(INTLLIBS) $(UTIL_LIB)

