#include <gnome.h>
#include <glib.h>
#include <stdio.h>
#include <unistd.h>

/* Do whatever you need to do... You should put the "expected" output of
   the program in ../expected/testname.out, which will be compared against
   the actual output of the test when it is run.

   A non-zero exit code also indicates failure.
*/

int main(int argc, char *argv[])
{
	gint outfd, readlen;
	gchar abuf[128];

	gnomelib_init(&argc, &argv);
	outfd = gnome_file_convert("fileconvert.in",
				   "text/html",
				   "text/ascii");

	while((readlen = read(outfd, abuf, sizeof(abuf) - 1))
		== sizeof(abuf) - 1) {
		write(1, abuf, readlen);
	}

	printf("\n");
	fflush(stdout); /* Make sure to fflush things after you do
			   any output, so what you see on your terminal
			   is the same as what goes into the output file */
	return 0;
}
