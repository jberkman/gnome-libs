#Utilitiesdir = $(datadir)/apps/Utilities

INCLUDES = -I.. -I$(srcdir)/.. -I$(includedir) @SUPPORTINCS@ \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-DLOCALEDIR=\"$(datadir)/locale\"

LDADD = \
	$(top_builddir)/libgnomeui/libgnomeui.la 	\
	$(top_builddir)/libgnome/libgnome.la		\
	@LIBSUPPORT@					\
	$(LIBGTKXMHTML_LIBS) $(LIBGNOME_LIBS) $(LIBGNOMEUI_LIBS) $(INTLLIBS)

noinst_PROGRAMS =  \
	gnome-hello-0-basic 		\
	gnome-hello-1-menus 		\
	gnome-hello-2-i18n  		\
	gnome-hello-3-parse-args	\
	gnome-hello-4-SM		\
	gnome-hello-5-dnd

# automake does not yet dig .sgml files, so for now the DocBook
# files must be extra_dist.  Also, distribute the `.html' file
# so that people without DocBook can still use the docs.
EXTRA_DIST = gnome-hello.sgml

