sbin_PROGRAMS    = gnome-pty-helper
lib_LTLIBRARIES  = libzvt.la
noinst_PROGRAMS  = zterm test-utmp

libzvtincludedir = $(includedir)/zvt

INCLUDES = -I.. -I$(srcdir)/.. $(SUPPORTINCS)		\
	-I$(top_srcdir)/intl -I$(top_builddir)/intl	\
	-I$(includedir) $(GTK_CFLAGS)  			\
	$(GDK_IMLIB_CFLAGS)				\
	-DGNOMESBINDIR=\""$(sbindir)"\"

libzvt_la_SOURCES =					\
	gnome-login-support.h				\
	gnome-login-support.c				\
	lists.c						\
	subshell.h					\
	subshell.c					\
	subshell-includes.h				\
	update.c					\
	vt.c						\
	zvtterm.c

libzvt_la_LDFLAGS = -version-info 1:0:0 -rpath $(libdir)

libzvtinclude_HEADERS =					\
	lists.h						\
	vt.h						\
	vtx.h						\
	zvtterm.h

gnome_pty_helper_SOURCES = 				\
	gnome-pty.h					\
	gnome-pty-helper.c				\
	gnome-login-support.c				\
	gnome-utmp.c					\
	gnome-login-support.h

gnome_pty_helper_LDADD =				\
	$(UTIL_LIBS)

EXTRA_DIST = TODO BUGS README 

zterm_SOURCES	= zterm.c

zterm_LDADD	= 					\
	$(top_builddir)/zvt/libzvt.la			\
	$(GTK_LIBS) 					\
	$(GDK_IMLIB_LIBS)				\
	@LIBSUPPORT@ $(INTLLIBS)

test_utmp_SOURCES =					\
	gnome-utmp.c					\
	test-utmp.c

test_utmp_LDADD =					\
	$(UTIL_LIBS)

install-exec-local:
	-chown root.root $(sbindir)/gnome-pty-helper
	-chmod +s $(sbindir)/gnome-pty-helper
