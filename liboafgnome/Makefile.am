bin_PROGRAMS=gnome-remote-bootstrap

lib_LTLIBRARIES=liboafgnome.la

INCLUDES=-DVERSION=\"$(VERSION)\" \
	-I$(top_srcdir) -I$(top_builddir) \
	-I$(top_srcdir)/oaf -I$(top_builddir)/oaf \
	 $(GTK_CFLAGS) $(ORBIT_CFLAGS)

liboafgnome_la_LDFLAGS=-L`cd $(top_builddir); pwd`/libgnomeui/.libs -lgnomeui \
	 -L`cd $(top_builddir); pwd`/liboaf/.libs -loaf $(GTK_LIBS)

liboafgnomeincludedir=$(includedir)/liboaf
liboafgnomeinclude_HEADERS=oafgnome.h
liboafgnome_la_SOURCES=oafgnome.c

gnome_remote_bootstrap_SOURCES=gnome-remote-bootstrap.c
gnome_remote_bootstrap_LDADD=../libgnome/libgnome.la
