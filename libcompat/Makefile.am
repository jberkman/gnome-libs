## Process this file with automake to produce Makefile.in

LIB_VERSION=0:0:0
LIB_VERSION_NUM=0.0.0

lib_LIBRARIES = libgnome-1-compat-2.a

INCLUDES = \
	-I$(top_builddir)				\
	-I$(top_srcdir)					\
	-I$(top_srcdir)/intl 				\
	-I$(top_builddir)/intl				\
	-I$(top_srcdir)/libgnome 			\
	-I$(top_builddir)/libgnome			\
	-I$(top_srcdir)/libgnomeui 			\
	-I$(top_builddir)/libgnomeui			\
	$(SUPPORTINCS) 					\
	$(GLIB_CFLAGS)					\
	$(GTK_CFLAGS)					\
	$(LIBART_CFLAGS)				\
	-I$(includedir)					\
	$(AUDIOFILE_CFLAGS)				\
	$(ESD_CFLAGS)					\
        -DGNOMELIBDIR=\""$(libdir)"\"			\
        -DGNOMEDATADIR=\""$(datadir)"\"			\
        -DGNOMEBINDIR=\""$(bindir)"\"			\
        -DGNOMELOCALSTATEDIR=\""$(localstatedir)"\"	\
        -DGNOMELOCALEDIR=\""$(gnomelocaledir)"\"	\
	-DG_LOG_DOMAIN=\"GnomeCompat\"

libgnome1compatincludedir = $(includedir)/gnome/$(GNOME_INTERFACE_VERSION)/compat/1

libgnome_1_compat_2_a_SOURCES =\
	gnome-guru.c			\
	gnome-spell.c			\
	gnome-startup.c			\
	gnomeui10-compat.c		\
	gnome-dns.c			\
	gnome-dentry-edit.c		\
	gnome-druid-page-start.c	\
	gnome-druid-page-finish.c	\
	gnome-procbar.c			\
	gnome-properties.c		\
	gnome-property-entries.c	\
	gnome-file-entry.c		\
	gnome-pixmap-entry.c		\
	gnome-number-entry.c		\
	gnome-calculator.c		\
	gnome10-compat.c		\
	gnome-help.c			\
	gnome-dentry.c			\
	gnome-popt.c			\
	gnome-history.c

# This probably won't work right
#	gnome-canvas-image.c

libgnome1compatinclude_HEADERS = \
	libgnomeui-compat-1.0.h		\
	gnome-font-selector.h 		\
	gnome-guru.h			\
	gnome-spell.h			\
	gnome-startup.h			\
	gnome-dentry-edit.h		\
	gnomeui10-compat.h		\
	gnome-dns.h			\
	gnome-druid-page-start.h	\
	gnome-druid-page-finish.h	\
	gnome-procbar.h			\
	gnome-properties.h		\
	gnome-property-entries.h	\
	gnome-font-selector.h		\
	gnome-file-entry.h		\
	gnome-pixmap-entry.h		\
	gnome-number-entry.h		\
	gnome-calculator.h		\
	libgnome-compat-1.0.h		\
	gnome10-compat.h		\
	gnome-help.h			\
	gnome-dentry.h			\
	gnome-popt.h			\
	gnome-history.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libgnome-1-compat-2.0.pc

EXTRA_DIST = libgnome-1-compat-2.0.pc.in
