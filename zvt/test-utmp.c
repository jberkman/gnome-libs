/*
 * Use this program to test your implementation of update_dbs
 */
#include <unistd.h>
#include "gnome-pty.h"

int
main ()
{
void *utmp;
	utmp = update_dbs (1, 1, "testlogin", ":0", "/dev/ttyp9");
	sleep (120);
	write_logout_record (utmp, 1, 1);
	return 0;
}
