SUBDIRS = ui-guide

htmldir = $(prefix)/html
html_DATA = gnome-dev-info.html

gnome-dev-info.html: gnome-dev-info.sgml libgnomeui.sgml libgnome.sgml
	-db2html gnome-dev-info.sgml

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

# automake does not yet dig .sgml files, so for now the DocBook
# files must be extra_dist.  Also, distribute the `.html' file
# so that people without DocBook can still use the docs.
EXTRA_DIST = \
	gnome-canvs.sgml	\
	gnome-dev-info.html	\
	gnome-dev-info.sgml	\
	libgnomeui.sgml		\
	libgnome.sgml
