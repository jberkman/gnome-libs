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

# automake does not yet dig .sgml files, so for now the DocBook files
# must be extra_dist.  Also, in the future we should distribute the
# HTML files so that people without DocBook can still use the docs.
EXTRA_DIST = gnome-hello.sgml
GNOME_HELLO_GIFS=gnome-hello-1-main-screen.gif gnome-hello-1-file-menu.gif \
	gnome-hello-1-help-menu.gif gnome-hello-1-about-dialog.gif

manual-html: gnome-hello/c01.html
manual-ps: gnome-hello.ps
manual-rtf: gnome-hello.rtf
manual-all: manual-html manual-ps manual-rtf

gnome-hello/c01.html: gnome-hello.sgml
	db2html gnome-hello.sgml
	cp $(GNOME_HELLO_GIFS) gnome-hello
gnome-hello.ps: gnome-hello.sgml
	db2ps gnome-hello.sgml
gnome-hello.rtf: gnome-hello.sgml
	db2rtf gnome-hello.sgml
