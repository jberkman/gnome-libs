## Process this file with automake to produce Makefile.in

LINK = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) -o $@

libgnomeincludedir = $(includedir)/libgnome

INCLUDES = -I.. -I$(srcdir)/.. -I$(includedir) @SUPPORTINCS@ \
        @GTK_CFLAGS@  \
	-DGNOMELIBDIR=\""$(libdir)"\" \
	-DGNOMEDATADIR=\""$(datadir)"\" \
	-DGNOMEBINDIR=\""$(bindir)"\" \
	-DGNOMELOCALSTATEDIR=\""$(localstatedir)"\" \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-I$(top_srcdir)/intl \
	-I$(srcdir)/../programs \
	-I$(top_srcdir)/support

lib_LTLIBRARIES = libgnomerasterapi.la

libgnomerasterapi_la_SOURCES = \
	gnome-rasterapi.c

libgnomeinclude_HEADERS = \
	gnome-rasterapi.h

noinst_HEADERS =

EXTRA_DIST = README

libgnomerasterapi_la_LDFLAGS = -version-info 0:0:0 -rpath $(libdir)
libgnomerasterapi_la_LIBADD = -lglib -lm $(DL_LIB)

#bin_PROGRAMS = dns-helper

#dns_helper_SOURCES = dns-helper.c
#dns_helper_LDADD = $(NSL_LIBS)

install-data-local:
	




