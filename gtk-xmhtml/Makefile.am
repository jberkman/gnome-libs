## Procesed by automake to procduce Makefile.in

libgtkxmhtmlincludedir = $(includedir)/gtk-xmhtml

INCLUDES = -I.. -I$(srcdir)/.. -I$(includedir) -DWITH_GTK -DDEBUG -DGNOME_LIB


lib_LTLIBRARIES = libgtkxmhtml.la

libgtkxmhtml_la_SOURCES =	\
	XmHTML.c		\
	gtk-error.c		\
	gtk-forms.c		\
	frames.c		\
	parse.c			\
	StringUtil.c		\
	fonts.c			\
	paint.c			\
	map.c			\
	callbacks.c		\
	format.c		\
	plc.c			\
	readXPM.c		\
	readJPEG.c		\
	readJPEGplc.c		\
	readBitmap.c		\
	readGIF.c		\
	LZWStream.c		\
	readGIFplc.c		\
	readFLG.c		\
	readPNG.c		\
	colors.c		\
	images.c		\
	quantize.c		\
	layout.c		\
	events.c		\
	debug.c			\
	XmHTML-motif.h		\
	XmHTMLfuncs.h		\
	debug.h			\
	escapes.h		\
	motif.h			\
	plc.h

libgtkxmhtmlinclude_HEADERS =	\
	miniparse.h		\
	HTML.h			\
	XmHTML.h		\
	XmHTMLI.h		\
	XmHTMLP.h		\
	gtk-xmhtml-p.h		\
	gtk-xmhtml.h		\
	toolkit.h

libgtkxmhtml_la_LDFLAGS = -version-info 0:0:0 -rpath $(libdir)
libgtkxmhtml_la_LIBADD = $(GTK_LIBS) $(X_LIBS) $(x_libs)

bin_PROGRAMS = htmltest

htmltest_SOURCES = test.c
htmltest_LDADD   = $(top_builddir)/gtk-xmhtml/libgtkxmhtml.la \
	$(GTK_LIBS) $(X_LIBS) $(x_libs) $(Z_LIBS) $(JPEG_LIBS) $(PNG_LIBS) $(XPM_LIBS)

## These are included by XmHTML.c:

EXTRA_DIST = gtk-xmhtml.c XmHTML-motif.c

## If I dont do this, for some wierdo reason, it tries to compile gtk-xmhtml.o
gtk-xmhtml:
	echo
