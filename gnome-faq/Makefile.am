##
## This Makefile.am is used both in gnome-libs and gnome-docu.
## There is a `INSIDE_GNOME_LIBS' automake conditional which is true in
## gnome-libs and false in gnome-docu.
##
## April 10,
## Martin Baulig
##

EXTRA_DIST	= gnome-faq.sgml
SUFFIXES	= .tar.gz .sgml

if INSIDE_GNOME_LIBS

# Make rules for the `gnome-libs' module

htmldir = $(prefix)/html
html_DATA = gnome-faq.html

gnome-faq.html: gnome-faq/index.html
	-cp $(srcdir)/gnome-faq/index.html gnome-faq.html

gnome-faq/index.html: gnome-faq.sgml
	-(cd $(srcdir); db2html gnome-faq.sgml) > /dev/null

dist-hook:
	tar cvf - gnome-faq | (cd $(distdir); tar xf -)

else

# Make rules for the `gnome-docu' module

htmldir		= $(datadir)/html

html_subdirs	= gnome-faq

# Well, yes - you are using GNU Make, aren't you ... ?
tgz_FILES	= $(addsuffix .tar.gz,$(html_subdirs))
sgml_FILES	= $(addsuffix .sgml,$(html_subdirs))

noinst_DATA	= $(tgz_FILES)

CLEANFILES	= $(tgz_FILES)

include $(TOPDIR)/scripts/Makefile.Common

distclean-local:
	$(MAKE) distclean-do-local

install-data-local:
	$(MAKE) install-data-do-local

endif
