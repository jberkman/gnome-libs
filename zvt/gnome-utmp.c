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

#if !defined(UTMP_FILENAME)
#    if defined(UTMP_FILE)
#        define UTMP_FILENAME UTMP_FILE
#    elif defined(_PATH_UTMP) /* BSD systems */
#        define UTMP_FILENAME _PATH_UTMP
#    else
#        define UTMP_FILENAME "/etc/utmp"
#    endif
#endif

#if !defined(WTMP_FILENAME)
#    if defined(WTMP_FILE)
#        define WTMP_FILENAME WTMP_FILE
#    elif defined(_PATH_WTMP) /* BSD systems */
#        define WTMP_FILENAME _PATH_WTMP
#    else
#        define WTMP_FILENAME "/etc/wtmp"
#    endif
#endif

#if defined(HAVE_GETUTMPX)
#undef  WTMP_FILENAME
#define WTMP_FILENAME WTMPX_FILE
#define update_wtmp updwtmpx
#else /* !HAVE_GETUTMPX */

static void
update_wtmp (char *file, struct utmp *putmp)
{
	int fd, times = 10;
#if defined(HAVE_FCNTL)
	struct flock lck;

	lck.l_whence = SEEK_END;
	lck.l_len    = 0;
	lck.l_start  = 0;
	lck.l_type   = F_WRLCK;
#endif
	
	if ((fd = open (file, O_WRONLY|O_APPEND, 0)) < 0)
		return;

	while (times--)
#if defined(HAVE_FCNTL)
	    if ((fcntl (fd, F_SETLK, &lck) < 0)){
		if (errno != EAGAIN && errno != EACCES){
#elif defined(HAVE_FLOCK)
	    if (flock(fd, LOCK_EX | LOCK_NB) < 0){
		if (errno != EWOULDBLOCK){
#endif
		    close (fd);
		    return;
		    }
		sleep (1); /*?!*/
		} else
			break;

	lseek (fd, 0, SEEK_END);
	write (fd, putmp, sizeof(struct utmp));

	/* unlocking the file */
#if defined(HAVE_FCNTL)	
	lck.l_type = F_UNLCK;
	fcntl (fd, F_SETLK, &lck);
#elif defined(HAVE_FLOCK)
	flock (fd, LOCK_UN);
#endif
	close (fd);
}
#endif /* !HAVE_GETUTMPX */

#if !defined(HAVE_GETUTENT)
static void
update_utmp (char *file, struct utmp *putmp)
{
    int fd, tty;

    tty = ttyslot();
    if (tty > 0 && (fd = open(file, O_WRONLY|O_CREAT, 0644)) < 0)
	return;
	
    lseek(fd, (off_t)(tty * sizeof(struct utmp)), SEEK_SET);
    write(fd, putmp, sizeof(struct utmp));
    close(fd);
}
#endif /* !HAVE_GETUTENT */

void 
write_logout_record (void *data, int utmp, int wtmp)
{
	UTMP *ut = data;
	struct utmp *wut;

#if defined(HAVE_GETUTENT)
	utmpname (UTMP_FILENAME);
	setutent ();

	if((wut = getutline(ut)) == NULL)
	    return;

#else
	if((wut = malloc(sizeof(UTMP))) == NULL)
	    return;

	memset (wut, 0, sizeof(UTMP));

	strncpy (wut->ut_line, ut->ut_line, sizeof (ut->ut_line));
#endif /* !HAVE_GETUTENT */

#if defined(HAVE_UT_UT_TYPE)
	wut->ut_type = DEAD_PROCESS;
#endif
#if defined(HAVE_UT_UT_PID)
	wut->ut_pid  = 0;
#endif
#if defined(HAVE_UT_UT_USER)
	memset (wut->ut_user, 0, sizeof (wut->ut_user));
#endif
#if defined(HAVE_UT_UT_TIME)
	wut->ut_time = time (NULL);
#endif

	if (wtmp)
	    update_wtmp (WTMP_FILENAME, wut);

#if defined(HAVE_GETUTENT)
	if (utmp)
	    pututline (wut);

#if defined(HAVE_GETUTMPX)
	getutmpx (ut, wut);
#endif
	endutent ();
#else
	if (utmp)
	    update_utmp (UTMP_FILENAME, wut);
	    
	free (wut);
#endif /* !HAVE_GETUTENT */
	free (data); /* ?! */
}

void *
update_dbs (int utmp, int wtmp, char *login_name, char *display_name, char *term_name)
{
	UTMP *ut;
	struct utmp ut_aux;
	struct timeval tv;
	char *pty = term_name;

	ut = (UTMP *) malloc (sizeof (UTMP));
	if (!ut)
		return;
	
	memset (ut, 0, sizeof (UTMP));

#if defined(HAVE_GETUTENT)
	utmpname (UTMP_FILENAME);
	setutent();
#endif
#if defined(HAVE_UT_UT_PID)
	ut->ut_pid  = getppid ();
#endif
	if (strncmp (pty, "/dev/", 5) == 0)
		pty += 5;

#if defined(HAVE_UT_UT_ID)
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
#if defined(HAVE_UT_UT_TYPE)
	ut->ut_type = DEAD_PROCESS;
#endif
#if defined(HAVE_GETUTMPX)
	getutmp (ut, &ut_aux);
	getutid (&ut_aux);
#elif defined(HAVE_GETUTENT)
	getutid (ut);
#endif
#if defined(HAVE_UT_UT_TYPE)
	ut->ut_type = USER_PROCESS;
#endif

	strncpy (ut->ut_line, pty, sizeof (ut->ut_line));

#if defined(HAVE_UT_UT_NAME)
	strncpy (ut->ut_name, login_name, sizeof (ut->ut_name));
#endif
#if defined(HAVE_UT_UT_USER)
	strncpy (ut->ut_user, login_name, sizeof (ut->ut_user));
#endif
#if defined(HAVE_UT_UT_HOST)
	strncpy (ut->ut_host, display_name, sizeof (ut->ut_host));
	ut->ut_host [sizeof (ut->ut_host)-1] = 0;
#endif
#if defined(HAVE_UT_UT_TV)
	gettimeofday (&tv, NULL);
	ut->ut_tv = tv;
#endif
#if defined(HAVE_UT_UT_TIME)
	time (&ut->ut_time);
#endif

	if (wtmp)
		update_wtmp (WTMP_FILENAME, ut);

#if defined(HAVE_GETUTENT)
	if (utmp)
		pututline (ut);

	endutent ();
#else
	if (utmp)
		update_utmp (UTMP_FILENAME, ut);
#endif

	return ut;
}
