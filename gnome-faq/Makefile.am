EXTRA_DIST = \
	gnome-faq.sgml

if INSIDE_GNOME_LIBS

htmldir = $(prefix)/html
html_DATA = gnome-faq.html

gnome-faq.html: gnome-faq/index.html
	-cp $(srcdir)/gnome-faq/index.html gnome-faq.html

gnome-faq/index.html: gnome-faq.sgml
	-(cd $(srcdir); db2html gnome-faq.sgml) > /dev/null

dist-hook:
	tar cvf - gnome-faq | (cd $(distdir); tar xf -)

endif
