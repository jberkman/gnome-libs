htmldir = $(prefix)/html
html_DATA = gnome-dev-info.html

gnome-dev-info.html: gnome-dev-info.sgml libgnomeui.sgml libgnome.sgml
	-db2html gnome-dev-info.sgml

libgnomeui.sgml:
	if [ -x `which gdoc` ]; then \
		>libgnomeui.sgml; \
		for i in ../libgnomeui/*.h; do \
			gdoc/gdoc.pl $$i >> libgnomeui.sgml; \
		done; \
	fi

libgnome.sgml:
	if [ -x `which gdoc` ]; then \
		>libgnome.sgml; \
		for i in ../libgnome/*.h; do \
			gdoc/gdoc.pl $$i >> libgnome.sgml; \
		done; \
	fi

# automake does not yet dig .sgml files, so for now the DocBook
# files must be extra_dist.  Also, distribute the `.html' file
# so that people without DocBook can still use the docs.
EXTRA_DIST = gnome-dev-info.html gnome-dev-info.sgml \
		libgnomeui.sgml libgnome.sgml
