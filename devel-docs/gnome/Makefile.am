## Process this file with automake to produce Makefile.in
# The name of the module.
DOC_MODULE=gnome

# The top-level SGML file.
DOC_MAIN_SGML_FILE=gnome-docs.sgml

# The directory containing the source code (if it contains documentation).
DOC_SOURCE_DIR=$(GNOME_LIBS_DIR)/libgnome

HTML_DIR=$(datadir)/gnome/html

TARGET_DIR=$(HTML_DIR)/$(DOC_MODULE)

tmpl_sources =				\
	tmpl/gnome-config.sgml		\
	tmpl/gnome-defs.sgml		\
	tmpl/gnome-dentry.sgml		\
	tmpl/gnome-exec.sgml		\
	tmpl/gnome-fileconvert.sgml	\
	tmpl/gnome-help.sgml		\
	tmpl/gnome-history.sgml		\
	tmpl/gnome-i18n.sgml		\
	tmpl/gnome-metadata.sgml	\
	tmpl/gnome-mime-info.sgml	\
	tmpl/gnome-mime.sgml		\
	tmpl/gnome-paper.sgml		\
	tmpl/gnome-popt.sgml		\
	tmpl/gnome-regex.sgml		\
	tmpl/gnome-remote.sgml		\
	tmpl/gnome-score.sgml		\
	tmpl/gnome-sound.sgml		\
	tmpl/gnome-triggers.sgml	\
	tmpl/gnome-unused.sgml		\
	tmpl/gnome-url.sgml		\
	tmpl/gnome-util.sgml		\
	tmpl/libgnome.sgml

gnome_docdir = $(HTML_DIR)
gnome_doc_DATA = 			\
	gnome.html			\
	gnome-decl.txt			\
	gnome-sections.txt

EXTRA_DIST = $(gnome_doc_DATA)

gnome.html: html/book1.html
	cp html/book1.html gnome.html

html/book1.html: sgml/libgnome.sgml
	make html

sgml/libgnome.sgml: $(tmpl_sources)
	make sgml	

scan:
	gtkdoc-scan --module=$(DOC_MODULE) --source-dir=$(DOC_SOURCE_DIR) --ignore-headers="libgnomeP.h gnome-i18nP.h gnome-triggersP.h gnome-magic.h vroot.h"

templates: scan
	gtkdoc-mktmpl --module=$(DOC_MODULE)

sgml:
	gtkdoc-mkdb --module=$(DOC_MODULE) --source-dir=$(DOC_SOURCE_DIR)

html:
	if ! test -d html ; then mkdir html ; fi
	-cd html && gtkdoc-mkhtml $(DOC_MODULE) ../$(DOC_MAIN_SGML_FILE)

clean-local:
	rm -f *~ *.bak *.hierarchy *.signals *-unused.txt

maintainer-clean-local: clean
	rm -rf sgml html $(DOC_MODULE)-decl-list.txt $(DOC_MODULE)-decl.txt 

install-data-local: 
	install -d -m 0755 $(TARGET_DIR)
	-install -m 0644 html/*.html $(TARGET_DIR)
	-install -m 0644 html/index.sgml $(TARGET_DIR)
	-gtkdoc-fixxref --module=$(DOC_MODULE) --html-dir=$(HTML_DIR)

dist-hook:
	mkdir $(distdir)/html
	mkdir $(distdir)/sgml
	mkdir $(distdir)/tmpl
	-cp html/*.html html/*.css $(distdir)/html
	-cp tmpl/*.sgml $(distdir)/tmpl
	-cp sgml/*.sgml $(distdir)/sgml

.PHONY : html sgml templates scan

