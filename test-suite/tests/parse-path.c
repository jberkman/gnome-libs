#include <gnome.h>
#include <glib.h>
#include <stdio.h>

#include <config.h>

/* Do whatever you need to do... You should put the "expected" output of
   the program in ../expected/testname.out, which will be compared against
   the actual output of the test when it is run.

   A non-zero exit code also indicates failure.
*/

#ifndef TEST_INTERNALS

void
_test_suite_gnome_config_parse_path (const char *path, gint priv)
{
	g_error ("You need to run configure with the --enable-test-internals "
		 "flag to run this test.");
}

#else

extern void _test_suite_gnome_config_parse_path (const char *, gint);

#endif

static void
parse_path (const char *path)
{
	_test_suite_gnome_config_parse_path (path, 0);
	_test_suite_gnome_config_parse_path (path, 1);
}

static void
run_tests (void)
{
	parse_path ("section/key");
	parse_path ("section/key=default");

	parse_path ("/section/key");
	parse_path ("/section/key=default");

	parse_path ("file/section/key");
	parse_path ("file/section/key=default");

	parse_path ("/file/section/key");
	parse_path ("/file/section/key=default");

	parse_path ("=/file=/section/key");
	parse_path ("=/file=/section/key=default");

	parse_path ("=/file/section/key");
	parse_path ("=/file/section/key=default");

}

static void
run_tests_with_prefix (const char *prefix)
{
	gnome_config_push_prefix (prefix);
	run_tests ();
	gnome_config_pop_prefix ();
}

int
main (int argc, char *argv[])
{
	gnomelib_init("parse-path");

	gnome_user_dir = "/tmp/.gnome";
	gnome_user_private_dir = "/tmp/.gnome_private";

	run_tests ();

	run_tests_with_prefix ("prefix");
	run_tests_with_prefix ("prefix/");
	run_tests_with_prefix ("/prefix");
	run_tests_with_prefix ("/prefix/");

	run_tests_with_prefix ("prefix/a/b/c");
	run_tests_with_prefix ("prefix/a/b/c/");
	run_tests_with_prefix ("/prefix/a/b/c");
	run_tests_with_prefix ("/prefix/a/b/c/");

	run_tests_with_prefix ("=/prefix");
	run_tests_with_prefix ("=/prefix/");
	run_tests_with_prefix ("=/prefix/a/b/c");
	run_tests_with_prefix ("=/prefix/a/b/c/");

	return 0;
}
