/*
 * utmp/wtmp file updating
 *
 * Author:
 *    Miguel de Icaza (miguel@gnu.org).
 *
 * FIXME: Do we want to register the PID of the process running *under* the subshell
 * or the PID of the parent process? (we are doing the latter now).
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

#ifdef HAVE_GETUTMPX
#    include <utmpx.h>
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

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#define _HAVE_UT_HOST 1
#define _HAVE_UT_TIME 1
#endif

#ifdef USE_SYSV_UTMP
#ifdef HAVE_GETUTMPX
#    define UTMP struct utmpx
#    undef WTMP_FILENAME
#    define WTMP_FILENAME WTMPX_FILE
#    define update_wtmp updwtmpx
#else
#    define UTMP struct utmp

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

void *
update_dbs (char *login_name, char *display_name, char *term_name)
{
	UTMP *ut;
	struct utmp ut_aux;
	struct timeval tv;
	char *pty = term_name;

	ut = (UTMP *) malloc (sizeof (UTMP));
	memset (ut, 0, sizeof (*ut));
	
#ifdef _HAVE_UT_PID
	ut->ut_pid  = getppid ();
#endif
	if (strncmp (pty, "/dev/", 5) == 0)
		pty += 5;

#ifdef _HAVE_UT_ID
	/* BSD-like terminal name */
	if (strncmp (pty, "pty", 3) == 0 ||
	    strncmp (pty, "tty", 3) == 0)
		strncpy (ut->ut_id, pty+3, sizeof (ut->ut_id));
	else {
		if (strncmp (pty, "pts/", 4) == 0)
			sprintf (ut->ut_id, "vt%02x", atoi (pty+4));
		else
			ut->ut_id [0] = 0;
	}
#endif
#ifdef _HAVE_UT_TYPE
	ut->ut_type = DEAD_PROCESS;
#endif
#ifdef HAVE_GETUTMPX
	getutmp (ut, &ut_aux);
	getutid (&ut_aux);
#else
	getutid (ut);
#endif
	
#ifdef _HAVE_UT_TYPE
	ut->ut_type = USER_PROCESS;
#endif
	strncpy (ut->ut_name, login_name, sizeof (ut->ut_name));
#ifdef _HAVE_UT_USER
	strncpy (ut->ut_user, login_name, sizeof (ut->ut_user));
#endif
#ifdef _HAVE_UT_TV
	gettimeofday (&tv, NULL);
	ut->ut_tv = tv;
#endif
#ifdef _HAVE_UT_TIME
	time (&ut->ut_time);
#endif
#ifdef _HAVE_UT_HOST
	strncpy (ut->ut_host, display_name, sizeof (ut->ut_host));
#endif
	strncpy (ut->ut_line, pty, sizeof (ut->ut_line));

#ifdef HAVE_LOGIN
	login (ut);
#else
	utmpname (UTMP_FILENAME);
	pututline (ut);
	update_wtmp (WTMP_FILENAME, ut);
	endutent ();
#endif

	return ut;
}

#ifdef HAVE_GETUTMPX
void
write_logout_record (void *data)
{
	struct utmp utmp_aux;
	struct utmpx ut;

	utmpname (UTMP_FILENAME);
	setutent ();
	if (getutid (&ut) == NULL)
		return;
	ut.ut_type = DEAD_PROCESS;
#ifdef _HAVE_UT_TIME
	ut.ut_time = time (NULL);
#endif
	pututline (&ut);
	getutmpx (&ut, &utmp_aux);
	update_wtmp (WTMP_FILENAME, &ut);
	endutent ();
	
	free (data);
}
#else /* not HAVE_GETUTMPX */
void 
write_logout_record (void *data)
{
	struct utmp *ut = data;
	struct utmp *wut;
	
	utmpname (UTMP_FILENAME);
	setutent ();

	while ((wut = getutent ()) != NULL){
		if (strncmp (wut->ut_line, ut->ut_line, sizeof (ut->ut_line)) == 0){
			wut->ut_type = DEAD_PROCESS;
#ifdef _HAVE_UT_PID
			wut->ut_pid  = 0;
#endif
#ifdef _HAVE_UT_USER
			memset (wut->ut_user, 0, sizeof (wut->ut_user));
#endif
#ifdef _HAVE_UT_TIME
			wut->ut_time = time (NULL);
			
#endif
			pututline (wut);
			update_wtmp (WTMP_FILENAME, wut);
			break;
		}
	}
	endutent ();
}
#endif /* not HAVE_GETUTMPX */

#else /* Otherwise, use BSD-like utmp updating */

#endif /* USE_SYSV_UTMP */


