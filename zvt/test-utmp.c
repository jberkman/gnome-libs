/*
 * Use this program to test your implementation of update_dbs
 */
#include "gnome-pty.h"

int
main ()
{
	update_dbs ("testlogi", ":0", "/dev/ttyp4");
	return 0;
}
