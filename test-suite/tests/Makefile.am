noinst_PROGRAMS = fileconvert parse-path skel split trigs

INCLUDES =					\
	-I$(top_builddir) -I$(top_srcdir)	\
	$(GNOMESUPPORT_CFLAGS)			\
	$(GDK_IMLIB_CFLAGS)			\
	-I$(top_srcdir)/gdk-pixbuf              \
	$(GLIB_CFLAGS) $(GTK_CFLAGS)		\
	$(OAF_CFLAGS)				\
	$(LIBGLADE_CFLAGS)			\
	$(LIBART_CFLAGS)			\
	$(ORBIT_CFLAGS)				\
	$(GCONF_CFLAGS)

LDADD = $(top_builddir)/libgnomeui/libgnomeui-$(GNOME_INTERFACE_VERSION).la	\
	$(top_builddir)/libgnome/libgnome-$(GNOME_INTERFACE_VERSION).la		\
	$(LIBGNOMEUI_LIBS) @LIBART_LIBS@ $(INTLLIBS)

EXTRA_DIST = fileconvert.in lynxdump.sh types.cvt trigs.in
