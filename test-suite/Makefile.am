INCLUDES = 			\
	-I../.. 		\
	-I$(srcdir)/../.. 	\
	-I$(includedir)		\
	$(X_CFLAGS)

TESTS = runtest.sh

# Always run via sh so we don't have to worry about permissions.
TESTS_ENVIRONMENT = /bin/sh

EXTRA_DIST = $(TESTS)
