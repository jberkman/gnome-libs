sbin_PROGRAMS    = gnome-pty-helper
lib_LTLIBRARIES  = libzvt-2.la
noinst_PROGRAMS  = zterm test-utmp

libzvtincludedir = $(includedir)/zvt

INCLUDES = -I.. -I$(srcdir)/.. $(GNOMESUPPORT_CFLAGS)	\
	-I$(top_srcdir)/intl -I$(top_builddir)/intl	\
	-I$(top_srcdir)/gdk-pixbuf                      \
	$(GLIB_CFLAGS) $(GTK_CFLAGS)			\
	$(OAF_CFLAGS)					\
	$(LIBART_CFLAGS)				\
	$(ORBIT_CFLAGS)					\
	$(GCONF_CFLAGS)					\
	-DGNOMESBINDIR=\""$(sbindir)"\"

libzvt_2_la_SOURCES =		\
	gnome-login-support.h	\
	gnome-login-support.c	\
	lists.c			\
	subshell.h		\
	subshell.c		\
	subshell-includes.h	\
	update.c		\
	vt.c			\
	background.c		\
	zvtterm.c

libzvt_2_la_LDFLAGS = -version-info 0:0:0

libzvtinclude_HEADERS =		\
	lists.h			\
	vt.h			\
	vtx.h			\
	background.h		\
	zvtterm.h

#gnome_pty_helper_SOURCES = 	\
#	gnome-pty.h		\
#	gnome-pty-helper.c	\
#	gnome-login-support.c	\
#	gnome-utmp.c		\
#	gnome-login-support.h

#gnome_pty_helper_LDADD =	\
#	$(UTIL_LIBS)

# We really dont want $(LIBS) linked in with this
gnome-pty-helper: gnome-pty-helper.o gnome-login-support.o gnome-utmp.o
	$(CC) -o $@ $^ $(UTIL_LIBS)

# Since we do gnome-pty-helper manually, include the relevant sources here
EXTRA_DIST = TODO BUGS README	\
	gnome-pty.h gnome-pty-helper.c gnome-utmp.c

zterm_SOURCES	= zterm.c

zterm_LDADD	= 			\
	libzvt-$(GNOME_INTERFACE_VERSION).la \
	$(UTIL_LIBS)			\
	$(GTK_LIBS) 			\
	$(LIBART_LIBS)			\
	$(top_builddir)/libgnomeui/libgnomeui-$(GNOME_INTERFACE_VERSION).la \
	$(top_builddir)/libgnome/libgnome-$(GNOME_INTERFACE_VERSION).la	\
	$(INTLLIBS)

test_utmp_SOURCES =			\
	gnome-utmp.c			\
	test-utmp.c

test_utmp_LDADD =			\
	$(UTIL_LIBS)

install-exec-local:
	-file=$(DESTDIR)$(sbindir)/gnome-pty-helper; \
	chown root $$file && chgrp root $$file && chmod u+s $$file
