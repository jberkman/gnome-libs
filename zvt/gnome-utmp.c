/*
 * utmp/wtmp file updating
 *
 * Author:
 *    Miguel de Icaza (miguel@gnu.org).
 *    Timur I. Bakeyev (timur@gnu.org).
 *
 * FIXME: Do we want to register the PID of the process running *under* the subshell
 * or the PID of the parent process? (we are doing the latter now).
 *
 * FIXME: Solaris (utmpx) stuff need to be checked.
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
#include "gnome-login-support.h"

#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif

#if defined(HAVE_PATHS_H)
#    include <paths.h>
#endif

#if defined(HAVE_UTMPX_H)
#    include <utmpx.h>
#endif

#if defined(HAVE_TTYENT_H)
#    include <ttyent.h>
#endif

#if !defined(UTMP_OUTPUT_FILENAME)
#    if defined(UTMP_FILE)
#        define UTMP_OUTPUT_FILENAME UTMP_FILE
#    elif defined(_PATH_UTMP) /* BSD systems */
#        define UTMP_OUTPUT_FILENAME _PATH_UTMP
#    else
#        define UTMP_OUTPUT_FILENAME "/etc/utmp"
#    endif
#endif

#if !defined(WTMP_OUTPUT_FILENAME)
#    if defined(WTMPX_FILE)
#        define WTMP_OUTPUT_FILENAME WTMPX_FILE
#    elif defined(_PATH_WTMPX)
#        define WTMP_OUTPUT_FILENAME _PATH_WTMPX
#    elif defined(WTMPX_FILENAME)
#        define WTMP_OUTPUT_FILENAME WTMPX_FILENAME
#    elif defined(WTMP_FILE)
#        define WTMP_OUTPUT_FILENAME WTMP_FILE
#    elif defined(_PATH_WTMP) /* BSD systems */
#        define WTMP_OUTPUT_FILENAME _PATH_WTMP
#    else
#        define WTMP_OUTPUT_FILENAME "/etc/wtmp"
#    endif
#endif

#if defined(HAVE_UPDWTMPX)
#define update_wtmp updwtmpx
#elif defined(HAVE_UPDWTMP)
#define update_wtmp updwtmp
#else /* !HAVE_UPDWTMPX && !HAVE_UPDWTMP */
static void
update_wtmp (char *file, UTMP *putmp)
{
	int fd, times = 3;
#if defined(HAVE_FCNTL)
	struct flock lck;

	lck.l_whence = SEEK_END;
	lck.l_len    = 0;
	lck.l_start  = 0;
	lck.l_type   = F_WRLCK;
#endif
	
	if ((fd = open (file, O_WRONLY|O_APPEND, 0)) < 0)
		return;

#if defined(HAVE_FCNTL) || defined(HAVE_FLOCK)
	while (times--)
#    if defined(HAVE_FCNTL)
	    if ((fcntl (fd, F_SETLK, &lck) < 0)){
		if (errno != EAGAIN && errno != EACCES){
#    elif defined(HAVE_FLOCK)
	    if (flock(fd, LOCK_EX | LOCK_NB) < 0){
		if (errno != EWOULDBLOCK){
#    endif
		    close (fd);
		    return;
		    }
		sleep (1); /*?!*/
		} else
			break;
#endif /* HAVE_FCNTL || HAVE_FLOCK */

	lseek (fd, 0, SEEK_END);
	write (fd, putmp, sizeof(UTMP));

	/* unlock the file */
#if defined(HAVE_FCNTL)	
	lck.l_type = F_UNLCK;
	fcntl (fd, F_SETLK, &lck);
#elif defined(HAVE_FLOCK)
	flock (fd, LOCK_UN);
#endif
	close (fd);
}
#endif /* !HAVE_GETUTMPX */


#if defined(HAVE_GETUTMPX)
static void
update_utmp (UTMP *ut)
{
	/* struct utmp ut_aux; */
	
	setutxent();
	pututxline (ut);
	
	/* FIXME: Do we need this?
	   getutmp (ut, &ut_aux);
	   pututline (&ut_aux);
	*/
}
#elif defined(HAVE_GETUTENT)
static void
update_utmp (UTMP *ut)
{
	setutent();
	pututline (ut);
}
#elif defined(HAVE_GETTTYENT)
static void
update_utmp (UTMP *ut)
{
	struct ttyent *ty;
	int fd, pos = 0;
	
	if ((fd=open (UTMP_OUTPUT_FILENAME, O_RDWR|O_CREAT, 0644)) < 0) 
		return;
	
	setttyent ();
	while ((ty = getttyent ()) != NULL)
	{
		++pos;
		if (strncmp (ty->ty_name, ut->ut_line, sizeof (ut->ut_line)) == NULL)
		{
			lseek (fd, (off_t)(pos * sizeof(UTMP)), SEEK_SET);
			write(fd, ut, sizeof(UTMP));
		}
	}
	endttyent ();
	
	close(fd);
}
#endif

void 
write_logout_record (void *data, int utmp, int wtmp)
{
	UTMP put, *ut = data;

	memset (&put, 0, sizeof(UTMP));

#if defined(HAVE_UT_UT_TYPE)
	put.ut_type = DEAD_PROCESS;
#endif
#if defined(HAVE_UT_UT_ID)
	strncpy (put.ut_id, ut->ut_id, sizeof (put.ut_id));
#endif

	strncpy (put.ut_line, ut->ut_line, sizeof (put.ut_line));

#if defined(HAVE_UT_UT_TV)
	gettimeofday ((struct timeval*) &put.ut_tv, NULL);
#elif defined(HAVE_UT_UT_TIME)
	time (&put.ut_time);
#endif

	if (utmp)
		update_utmp (&put);

	if (wtmp)
		update_wtmp (WTMP_OUTPUT_FILENAME, &put);

	free (ut);
}

void *
write_login_record (char *login_name, char *display_name, char *term_name, int utmp, int wtmp)
{
	UTMP *ut;
	char *pty = term_name;

	if((ut=(UTMP *) malloc (sizeof (UTMP))) == NULL)
		return NULL;
	
	memset (ut, 0, sizeof (UTMP));

#if defined(HAVE_UT_UT_NAME)
	strncpy (ut->ut_name, login_name, sizeof (ut->ut_name));
#elif defined(HAVE_UT_UT_USER)
	strncpy (ut->ut_user, login_name, sizeof (ut->ut_user));
#endif

	/* This shouldn't happen */
	if (strncmp (pty, "/dev/", 5) == 0)
	    pty += 5;
	    
#if defined(HAVE_UT_UT_ID)
	/* BSD-like terminal name */
	if (strncmp (pty, "pty", 3) == 0 ||
	    strncmp (pty, "tty", 3) == 0)
		strncpy (ut->ut_id, pty+3, sizeof (ut->ut_id));
	else {
		int num;
		char buf[5];
		
		if (sscanf (pty, "%*[^0-9]%d", &num) == 1){
			sprintf (buf, "gt%02x", num);
			strncpy (ut->ut_id, buf, sizeof (ut->ut_id));
		} 
		else
			ut->ut_id [0] = '\0';
	}
#elif defined(HAVE_STRRCHR)
	{
		char *p;
		
		if ((p = strrchr (pty, '/')) != NULL) 
			pty = p + 1;
	}
#endif

	strncpy (ut->ut_line, pty, sizeof (ut->ut_line));

#if defined(HAVE_UT_UT_PID)
	ut->ut_pid  = getppid ();
#endif
#if defined(HAVE_UT_UT_TYPE)
	ut->ut_type = USER_PROCESS;
#endif
#if defined(HAVE_UT_UT_TV)
	gettimeofday ((struct timeval*) &(ut->ut_tv), NULL);
#elif defined(HAVE_UT_UT_TIME)
	time (&ut->ut_time);
#endif
#if defined(HAVE_UT_UT_HOST)
	strncpy (ut->ut_host, display_name, sizeof (ut->ut_host));
#    if defined(HAVE_UT_UT_SYSLEN)
	ut->ut_host [sizeof (ut->ut_host)-1] = '\0';
	ut->ut_syslen = strlen (ut->ut_host) + 1;
#    endif
#endif

	if (utmp)
		update_utmp (ut);

	if (wtmp)
		update_wtmp (WTMP_OUTPUT_FILENAME, ut);

	return ut;
}

void *
update_dbs (int utmp, int wtmp, char *login_name, char *display_name, char *term_name)
{
	return write_login_record (login_name, display_name, term_name, utmp, wtmp);
}
