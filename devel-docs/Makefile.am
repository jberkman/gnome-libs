#SUBDIRS = gnome gnomeui ui-guide templates 
SUBDIRS = ui-guide templates 

htmldir = $(prefix)/html
html_DATA = gnome-dev-info.html

gnome-dev-info.html: gnome-dev-info/book1.html
	-cp $(srcdir)/gnome-dev-info/book1.html gnome-dev-info.html

gnome-dev-info/book1.html: gnome-dev-info.sgml libgnomeui.sgml libgnome.sgml
	-(cd $(srcdir); db2html gnome-dev-info.sgml) > /dev/null

libgnomeui.sgml:
	if $(SHELL) -c 'gdoc -V > /dev/null 2>&1 < /dev/null'; then \
## Inputs and generated files all are in srcdir.
		cd $(srcdir); \
		: > libgnomeui.sgml; \
		for i in ../libgnomeui/*.h; do \
## FIXME: should use explicit perl invocation here.
			./gdoc/gdoc.pl $$i >> libgnomeui.sgml; \
		done; \
		cat trailer.sgml >> libgnomeui.sgml; \
	fi

libgnome.sgml:
	if $(SHELL) -c 'gdoc -V > /dev/null 2>&1 < /dev/null'; then \
## Inputs and generated files all are in srcdir.
		cd $(srcdir); \
		: > libgnome.sgml; \
## FIXME: should use explicit perl invocation here.
		for i in ../libgnome/*.h; do \
			./gdoc/gdoc.pl $$i >> libgnome.sgml; \
		done; \
		cat trailer.sgml >> libgnome.sgml; \
	fi

TEXT_FILES =					\
	README.gtkcauldron			\
	README.gtkcauldron_for_python		\
	adding-file-manager-new-items.txt	\
	adding-sounds.txt			\
	api-comment-style.txt			\
	gtk_dialog_cauldron.3			\
	suggestions.txt				\
	session-management.txt			\
	mime-type-handling.txt

# automake does not yet dig .sgml files, so for now the DocBook
# files must be extra_dist.  Also, distribute the `.html' file
# so that people without DocBook can still use the docs.
EXTRA_DIST = \
	gnome-doc-framework.txt		\
	gnome-app-helper.sgml		\
	gnome-canvas.sgml		\
	gnome-dev-info.html		\
	gnome-dev-info.sgml		\
	gnome-app-helper.sgml		\
	gnome-message-utils.sgml	\
	x-concepts.sgml			\
	libgnorba.sgml			\
	gnome-mdi.sgml			\
	libgnomeui.sgml			\
	libgnome.sgml			\
	$(TEXT_FILES)

docdir   = $(prefix)/doc
doc_DATA = $(TEXT_FILES)

dist-hook:
	tar cvf - gnome-dev-info | (cd $(distdir); tar xf -)

