GTK=/usr/local/include

CFLAGS = -I. -I$(GTK) -DVERSION=1 -g -Wall -DHAVE_JPEG -DHAVE_PNG -DHAVE_ZLIB

all: xmhtml test

xmhtml: Parser.o LZWStream.o StringUtil.o parse.o plc.o \
	readBitmap.o readFLG.o readXPM.o fonts.o paint.o \
	format.o map.o quantize.o  XmHTML.o frames.o numbers.o \
	readGIF.o readGIFplc.o readFLG.o colors.o gtk-error.o gtk-forms.o \
	readJPEG.o readJPEGplc.o readPNG.o callbacks.o \
	images.o 

XmImage.o:

HTMLOBJS = XmHTML.o gtk-error.o frames.o parse.o StringUtil.o fonts.o paint.o \
	map.o gtk-forms.o callbacks.o format.o plc.o numbers.o readXPM.o \
	readJPEG.o readJPEGplc.o readBitmap.o readGIF.o LZWStream.o readGIFplc.o \
	readFLG.o readPNG.o colors.o images.o quantize.o

test: test.o
	gcc -g -o xtest test.o $(HTMLOBJS)  -lXpm -lgtk -lgdk -lglib -lX11 -ljpeg -lpng -lz -L/usr/X11R6/lib -lm

test.o: test.c
	gcc -c test.c -I. -g

XmHTML.o: XmHTML.c gtk-xmhtml.c XmHTML.h

clean:
	rm *.o