## Process this file with automake to produce Makefile.in

# The name of the module.
DOC_MODULE=gnomeui

# The top-level SGML file.
DOC_MAIN_SGML_FILE=gnomeui-docs.sgml

# The directory containing the source code (if it contains documentation).
DOC_SOURCE_DIR=$(GNOME_LIBS_DIR)/libgnomeui

CFLAGS=`gnome-config --cflags gnomeui`
LDFLAGS=`gnome-config --libs gnomeui`

HTML_DIR=$(datadir)/gnome/html

TARGET_DIR=$(HTML_DIR)/$(DOC_MODULE)

tmpl_sources =					\
	tmpl/gnome-about.sgml			\
	tmpl/gnome-animator.sgml		\
	tmpl/gnome-app-helper.sgml		\
	tmpl/gnome-app-util.sgml		\
	tmpl/gnome-app.sgml			\
	tmpl/gnome-appbar.sgml			\
	tmpl/gnome-calculator.sgml		\
	tmpl/gnome-canvas-ellipse.sgml		\
	tmpl/gnome-canvas-group.sgml		\
	tmpl/gnome-canvas-image.sgml		\
	tmpl/gnome-canvas-item.sgml		\
	tmpl/gnome-canvas-line.sgml		\
	tmpl/gnome-canvas-polygon.sgml		\
	tmpl/gnome-canvas-rect-ellipse.sgml	\
	tmpl/gnome-canvas-rect.sgml		\
	tmpl/gnome-canvas-text.sgml		\
	tmpl/gnome-canvas-util.sgml		\
	tmpl/gnome-canvas-widget.sgml		\
	tmpl/gnome-canvas.sgml			\
	tmpl/gnome-client.sgml			\
	tmpl/gnome-color-picker.sgml		\
	tmpl/gnome-dateedit.sgml		\
	tmpl/gnome-dentry-edit.sgml		\
	tmpl/gnome-dialog-util.sgml		\
	tmpl/gnome-dialog.sgml			\
	tmpl/gnome-dns.sgml			\
	tmpl/gnome-dock-band.sgml		\
	tmpl/gnome-dock-item.sgml		\
	tmpl/gnome-dock-layout.sgml		\
	tmpl/gnome-dock.sgml			\
	tmpl/gnome-entry.sgml			\
	tmpl/gnome-file-entry.sgml		\
	tmpl/gnome-font-picker.sgml		\
	tmpl/gnome-font-selector.sgml		\
	tmpl/gnome-geometry.sgml		\
	tmpl/gnome-guru.sgml			\
	tmpl/gnome-href.sgml			\
	tmpl/gnome-ice.sgml			\
	tmpl/gnome-icon-entry.sgml		\
	tmpl/gnome-icon-item.sgml		\
	tmpl/gnome-icon-list.sgml		\
	tmpl/gnome-icon-sel.sgml		\
	tmpl/gnome-icon-text.sgml		\
	tmpl/gnome-init.sgml			\
	tmpl/gnome-less.sgml			\
	tmpl/gnome-mdi-child.sgml		\
	tmpl/gnome-mdi-generic-child.sgml	\
	tmpl/gnome-mdi-session.sgml		\
	tmpl/gnome-mdi.sgml			\
	tmpl/gnome-messagebox.sgml		\
	tmpl/gnome-number-entry.sgml		\
	tmpl/gnome-paper-selector.sgml		\
	tmpl/gnome-pixmap-entry.sgml		\
	tmpl/gnome-pixmap.sgml			\
	tmpl/gnome-popup-help.sgml		\
	tmpl/gnome-popup-menu.sgml		\
	tmpl/gnome-preferences.sgml		\
	tmpl/gnome-procbar.sgml			\
	tmpl/gnome-properties.sgml		\
	tmpl/gnome-property-entries.sgml	\
	tmpl/gnome-propertybox.sgml		\
	tmpl/gnome-scores.sgml			\
	tmpl/gnome-spell.sgml			\
	tmpl/gnome-startup.sgml			\
	tmpl/gnome-stock.sgml			\
	tmpl/gnome-types.sgml			\
	tmpl/gnome-uidefs.sgml			\
	tmpl/gnome-winhints.sgml		\
	tmpl/gnomeui-unused.sgml		\
	tmpl/gtk-clock.sgml			\
	tmpl/gtk-ted.sgml			\
	tmpl/gtkcauldron.sgml			\
	tmpl/gtkdial.sgml			\
	tmpl/gtkpixmapmenuitem.sgml

gnomeui_docdir = $(HTML_DIR)
gnomeui_doc_DATA =				\
	gnomeui.html				\
	gnomeui.hierarchy			\
	gnomeui.types				\
	gnomeui-scan.c				\
	gnomeui-decl.txt			\
	gnomeui-sections.txt

EXTRA_DIST = $(gnomeui_doc_DATA)

gnomeui.html: html/book1.html
	cp $(srcdir)/html/book1.html gnomeui.html

html/book1.html: sgml/gnomeui-doc.bottom
	make html

sgml/gnomeui-doc.bottom: $(tmpl_sources)
	make sgml

scan:
	-CFLAGS=$(CFLAGS) LDFLAGS=$(LDFLAGS) gtkdoc-scanobj --module=$(DOC_MODULE)
	gtkdoc-scan --module=$(DOC_MODULE) --source-dir=$(DOC_SOURCE_DIR)

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
	-cp sgml/*.bottom sgml/*.top $(distdir)/sgml

.PHONY : html sgml templates scan
