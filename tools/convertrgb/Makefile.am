INCLUDES =                      \
        -I$(includedir) $(GDK_IMLIB_CFLAGS)

bin_PROGRAMS = convertrgb

convertrgb_SOURCES = \
        convertrgb.c		\
        convertrgb.h

convertrgb_LDADD = \
	$(GDK_IMLIB_LIBS)

EXTRA_DIST = README

