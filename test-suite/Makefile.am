INCLUDES = 			\
	-I../.. 		\
	-I$(srcdir)/../.. 	\
	-I$(includedir)		\
	@GTK_CFLAGS@ @SUPPORTINCS@

LDADD = $(top_builddir)/libgnomeui/libgnomeui.la \
	$(top_builddir)/libgnome/libgnome.la \
	@LIBSUPPORT@ $(LIBGNOMEUI_LIBS) $(INTLLIBS)

# Hacky way of forcing `automake' to define $(COMPILE) and $(LINK)
BUILT_SOURCES=dummy.c
dummy.c:
	echo "int main(void){return 0;}" > dummy.c
if FALSE
noinst_PROGRAMS=dummy
endif

TESTS = runtest.sh

# Always run via sh so we don't have to worry about permissions.
TESTS_ENVIRONMENT = env \
		      srcdir="$(srcdir)" \
		      COMPILE="$(COMPILE)" \
		      LINK="$(LINK)" \
		      LDADD="$(LDADD)" \
		    $(SHELL) 

EXTRA_DIST = $(TESTS)
