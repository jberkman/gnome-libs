## Process this file with automake to produce Makefile.in

# The name of the module.
DOC_MODULE=zvt

# The top-level SGML file.
DOC_MAIN_SGML_FILE=zvt-docs.sgml

# The directory containing the source code (if it contains documentation).
DOC_SOURCE_DIR=$(GNOME_LIBS_DIR)/zvt

GNOME_DOC=${GNOME_LIBS_DIR}/tools/gnome-doc/gnome-doc

CFLAGS=`gnome-config --cflags gnome`
LDFLAGS="`gnome-config --libs zvt` -lgdk_imlib"

HTML_DIR=$(datadir)/gnome/html

TARGET_DIR=$(HTML_DIR)/$(DOC_MODULE)

scan:
	-CFLAGS=$(CFLAGS) LDFLAGS=$(LDFLAGS) gtkdoc-scanobj --module=$(DOC_MODULE)
	gtkdoc-scan --module=$(DOC_MODULE) --source-dir=$(DOC_SOURCE_DIR)

templates: scan
	gtkdoc-mktmpl --module=$(DOC_MODULE)

sgml:
	gtkdoc-mkdb --module=$(DOC_MODULE) --source-dir=$(DOC_SOURCE_DIR)

html:
	if ! test -d html ; then mkdir html ; fi
	if ! test -d sgml ; then mkdir sgml ; fi
	${GNOME_DOC} -module zvtterm -gnome \
		${DOC_SOURCE_DIR}/zvtterm.c > sgml/zvtterm.sgml
	-cd html && gtkdoc-mkhtml $(DOC_MODULE) ../$(DOC_MAIN_SGML_FILE)

clean-local:
	rm -f *~ *.bak *.hierarchy *.signals *-unused.txt

maintainer-clean-local: clean
	rm -rf sgml html $(DOC_MODULE)-decl-list.txt $(DOC_MODULE)-decl.txt 

install-data-local: html
	install -d -m 0755 $(TARGET_DIR)
	-install -m 0644 html/*.html $(TARGET_DIR)
	-install -m 0644 html/index.sgml $(TARGET_DIR)
	-gtkdoc-fixxref --module=$(DOC_MODULE) --html-dir=$(HTML_DIR)

.PHONY : html sgml templates scan
