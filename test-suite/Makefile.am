SUBDIRS = tests

TESTS = fileconvert.test \
	metadata.test \
	parse-path.test \
	skel.test \
	split.test \
	trigs.test

noinst_SCRIPTS = $(TESTS)

if MAINTAINER_MODE
%.test: 
	echo "the_test=$*" > $(srcdir)/$@; \
	echo ". $(srcdir)/runtest.sh" >> $(srcdir)/$@; \
	chmod +x $(srcdir)/$@
endif

# Always run via sh so we don't have to worry about permissions
TESTS_ENVIRONMENT = $(SHELL) 

EXTRA_DIST =					\
	runtest.sh				\
	expected/fileconvert.out		\
	expected/skel.out			\
	expected/split.out			\
	expected/trigs.out			\
	expected/metadata.out			\
	expected/parse-path.out data/sample	\
	$(TESTS)
