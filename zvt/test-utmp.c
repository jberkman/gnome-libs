/*
 * Use this program to test your implementation of update_dbs
 */
#include "gnome-pty.h"

int
main ()
{
#ifdef USE_SYSV_UTMP
	update_dbs ("testlogi", ":0", "/dev/ttyp4");
#endif
	return 0;
}
