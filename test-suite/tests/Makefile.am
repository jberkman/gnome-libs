noinst_PROGRAMS = fileconvert metadata parse-path skel split trigs

INCLUDES =					\
	-I$(top_builddir) -I$(top_srcdir)	\
	$(SUPPORTINCS)				\
	$(GDK_IMLIB_CFLAGS)			\
	-I$(top_srcdir)/gdk-pixbuf              \
	$(GTK_CFLAGS)

LDADD = $(top_builddir)/libgnomeui/libgnomeui-$(GNOME_INTERFACE_VERSION).la	\
	$(top_builddir)/libgnome/libgnome-$(GNOME_INTERFACE_VERSION).la		\
	$(LIBGNOMEUI_LIBS) @LIBART_LIBS@ $(INTLLIBS)

EXTRA_DIST = fileconvert.in lynxdump.sh types.cvt trigs.in
