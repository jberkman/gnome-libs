/*
 * utmp/wtmp file updating
 *
 * Author:
 *    Miguel de Icaza (miguel@gnu.org).
 */
 
#include <config.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <fcntl.h>
#include <utmp.h>
#include <errno.h>
#include "gnome-pty.h"

#include <sys/time.h>

#ifdef HAVE_LIBUTIL_H
#include <libutil.h>
#endif

#ifdef HAVE_PATHS_H
#    include <paths.h>
#endif
#ifdef HAVE_UTMPX_H
#    include <utmpx.h>
#    define USE_SYSV_UTMP
#else
#    ifdef HAVE_SETUTENT
#        define USE_SYSV_UTMP
#    endif
#endif

#ifndef UTMP_FILENAME
#    ifdef UTMP_FILE
#        define UTMP_FILENAME UTMP_FILE
#    elif defined _PATH_UTMP /* FreeBSD */
#        define UTMP_FILENAME _PATH_UTMP
#    else
#        define UTMP_FILENAME "/etc/utmp"
#    endif
#endif

#ifdef __FreeBSD__
#define _HAVE_UT_HOST 1
#define _HAVE_UT_TIME 1
#endif

#ifdef HAVE_UTMPX_H
#    undef WTMP_FILENAME
#    define WTMP_FILENAME WTMPX_FILE
#    define update_wtmp updwtmpx
#else
static void
update_wtmp (char *file, struct utmp *putmp)
{
	int fd, times = 10;
	struct flock lck;
	
	if ((fd = open (file, O_WRONLY|O_APPEND, 0)) < 0)
		return;
	
	lck.l_whence = SEEK_END;
	lck.l_len    = 0;
	lck.l_start  = 0;
	lck.l_type   = F_WRLCK;
	
	while (times--)
		if ((fcntl (fd, F_SETLK, &lck) < 0)){
			if (errno != EAGAIN){
				close (fd);
				return;
			}
		} else
			break;

	write (fd, putmp, sizeof(struct utmp));
	
	/* unlocking the file */
	lck.l_type = F_UNLCK;
	fcntl (fd, F_SETLK, &lck);
	
	close (fd);
}
#endif

#ifndef WTMP_FILENAME
#    ifdef WTMP_FILE
#        define WTMP_FILENAME WTMP_FILE
#    elif defined _PATH_WTMP /* FreeBSD */
#        define WTMP_FILENAME _PATH_WTMP
#    else
#        define WTMP_FILENAME "/etc/wtmp"
#    endif
#endif

void
update_dbs (char *login_name, char *display_name, char *term_name)
{
	struct utmp ut;
	struct timeval tv;
	char *pty = term_name;

	memset (&ut, 0, sizeof (ut));
	
#ifdef _HAVE_UT_TYPE
	ut.ut_type = USER_PROCESS;
#endif
#ifdef _HAVE_UT_PID
	ut.ut_pid  = getppid ();
#endif
	if (strncmp (pty, "/dev/", 5) == 0)
		pty += 5;

#ifdef _HAVE_UT_ID
	/* BSD-like terminal name */
	if (strncmp (pty, "pty", 3) == 0 ||
	    strncmp (pty, "tty", 3) == 0)
		strncpy (ut.ut_id, pty+3, sizeof (ut.ut_id));
	else {
		if (strncmp (pty, "pts/", 4) == 0)
			sprintf (ut.ut_id, "vt%02x", atoi (pty+4));
		else
			ut.ut_id [0] = 0;
	}
#endif
	fprintf (stderr, "Poneidno: %s\n", pty);
	strncpy (ut.ut_name, login_name, sizeof (ut.ut_name));
#ifdef _HAVE_UT_USER
	strncpy (ut.ut_user, login_name, sizeof (ut.ut_user));
#endif
#ifdef _HAVE_UT_TV
	gettimeofday (&tv, NULL);
	ut.ut_tv = tv;
#endif
#ifdef _HAVE_UT_TIME
	time (&ut.ut_time);
#endif
#ifdef _HAVE_UT_HOST
	strncpy (ut.ut_host, display_name, sizeof (ut.ut_host));
#endif
	strncpy (ut.ut_line, pty, sizeof (ut.ut_line));

#ifdef HAVE_LOGIN
	login (&ut);
#else
	utmpname (UTMP_FILENAME);
	pututline (&ut);
	update_wtmp (WTMP_FILENAME, &ut);
	endutent ();
#endif
}
