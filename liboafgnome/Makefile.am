bin_PROGRAMS=gnome-remote-bootstrap

lib_LTLIBRARIES=liboafgnome.la

INCLUDES=-DVERSION=\"$(VERSION)\" \
	-I$(top_srcdir) -I$(top_builddir) \
	-I$(top_srcdir)/oaf -I$(top_builddir)/oaf \
	 $(GTK_CFLAGS) $(ORBIT_CFLAGS)

liboafgnome_la_LDFLAGS=-L`cd $(top_builddir); pwd`/libgnomeui/.libs -lgnomeui \
	 -L`cd $(top_builddir); pwd`/liboaf/.libs -loaf $(GTK_LIBS)

gnome_factory_built=\
	gnome-factory.h 				\
	gnome-factory-skels.c 				\
	gnome-factory-stubs.c 				\
	gnome-factory-common.c

gnome_unknown_built = \
	gnome-unknown.h					\
	gnome-unknown-skels.c				\
	gnome-unknown-stubs.c				\
	gnome-unknown-common.c

history_db_built = \
	HistoryDB.h \
	HistoryDB-stubs.c \
	HistoryDB-skels.c \
	HistoryDB-common.c

$(gnome_factory_built): $(top_srcdir)/idl/gnome-factory.idl
	$(ORBIT_IDL) $(IDL_FLAGS) $(top_srcdir)/idl/gnome-factory.idl

$(gnome_unknown_built): $(top_srcdir)/idl/gnome-unknown.idl
	$(ORBIT_IDL) $(IDL_FLAGS) $(top_srcdir)/idl/gnome-unknown.idl

$(history_db_built): $(top_srcdir)/idl/HistoryDB.idl
	$(ORBIT_IDL) $(IDL_FLAGS) $(top_srcdir)/idl/HistoryDB.idl

gnorba_built = $(gnome_factory_built) $(gnome_unknown_built) $(history_db_built)

liboafgnomeincludedir=$(includedir)/liboaf
liboafgnomeinclude_HEADERS=oafgnome.h gnome-unknown.h gnome-factory.h HistoryDB.h
liboafgnome_la_SOURCES=oafgnome.c $(gnorba_built)

ORBIT_IDL=orbit-idl

IDL_FLAGS=-D__LIBOAFGNOME_COMPILATION

BUILT_SOURCES=$(gnorba_built)

gnome_remote_bootstrap_SOURCES=gnome-remote-bootstrap.c
gnome_remote_bootstrap_LDADD=../libgnome/libgnome.la
