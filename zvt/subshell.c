/* {{{ Copyright notice */

/* Concurrent shell support for the Midnight Commander
   Copyright (C) 1994, 1995, 1998 Dugan Porter

   This program is free software; you can redistribute it and/or
   modify it under the terms of Version 2 of the GNU General Public
   License, as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* }}} */

#include <config.h>

/* {{{ Declarations */

#include <stdio.h>      
#include <stdlib.h>	/* For errno, putenv, etc.	      */
#include <errno.h>	/* For errno on SunOS systems	      */
#include <termios.h>	/* tcgetattr(), struct termios, etc.  */
#if (!defined(__IBMC__) && !defined(__IBMCPP__))
#include <sys/types.h>	/* Required by unistd.h below	      */
#endif
#include <sys/ioctl.h>	/* For ioctl() (surprise, surprise)   */
#include <fcntl.h>	/* For open(), etc.		      */
#include <string.h>	/* strstr(), strcpy(), etc.	      */
#include <signal.h>	/* sigaction(), sigprocmask(), etc.   */
#ifndef SCO_FLAVOR
#	include <sys/time.h>	/* select(), gettimeofday(), etc.     */
#endif /* SCO_FLAVOR */
#include <sys/stat.h>	/* Required by dir.h & panel.h below  */
#include <sys/param.h>	/* Required by panel.h below	      */

#include "subshell.h"

/*#ifdef HAVE_UNISTD_H*/
#   include <unistd.h>	/* For pipe, fork, setsid, access etc */
/*#endif*/

#ifdef HAVE_SYS_SELECT_H
#   include <sys/select.h>
#endif

/*#ifdef HAVE_SYS_WAIT_H*/
#   include <sys/wait.h> /* For waitpid() */
/*#endif*/

#ifndef WEXITSTATUS
#   define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif

#ifndef WIFEXITED
#   define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif

#ifdef HAVE_GRANTPT
#   include <stropts.h> /* For I_PUSH			      */
#else
#   include <grp.h>	/* For the group struct & getgrnam()  */
#endif

#ifdef SCO_FLAVOR
#   include <grp.h>	/* For the group struct & getgrnam()  */
#endif /* SCO_FLAVOR */

/* Local functions */
static int pty_open_master (char *pty_name);
static int pty_open_slave (const char *pty_name);

/* }}} */
/* {{{ Definitions */

#ifndef STDIN_FILENO
#    define STDIN_FILENO 0
#endif

#ifndef STDOUT_FILENO
#    define STDOUT_FILENO 1
#endif

#ifndef STDERR_FILENO
#    define STDERR_FILENO 2
#endif

/* {{{ init_subshell */

/*
 *  Fork the subshell, and set up many, many things.
 *
 */

int init_subshell (int *master, char *pty_name)
{
    /* {{{ Local variables */

  int pty_slave;
  int subshell_pty;
  int subshell_pid;

    /* }}} */

    /* }}} */
    /* {{{ Open a pty for talking to the subshell */

    /* FIXME: We may need to open a fresh pty each time on SVR4 */
    
    subshell_pty = pty_open_master (pty_name);
    if (subshell_pty == -1)
      {
	return -1;
      }
    pty_slave = pty_open_slave (pty_name);
    if (pty_slave == -1)
      {
	return -1;
      }


    /* }}} */
    /* {{{ Fork the subshell */

    subshell_pid = fork ();
    
    if (subshell_pid == -1)
    {
      return -1;
    }

   /* }}} */

    if (subshell_pid == 0)  /* We are in the child process */
    {
	setsid ();  /* Get a fresh terminal session */

	/* {{{ Open the slave side of the pty: again */
	pty_slave = pty_open_slave (pty_name);

	/* This must be done before closing the master side of the pty, */
	/* or it will fail on certain idiotic systems, such as Solaris.	*/

	/* Close master side of pty.  This is important; apart from	*/
	/* freeing up the descriptor for use in the subshell, it also	*/
	/* means that when MC exits, the subshell will get a SIGHUP and	*/
	/* exit too, because there will be no more descriptors pointing	*/
	/* at the master side of the pty and so it will disappear.	*/

	close (subshell_pty);

	/* }}} */
	/* {{{ Make sure that it has become our controlling terminal */

	/* Redundant on Linux and probably most systems, but just in case: */

#	ifdef TIOCSCTTY
	ioctl (pty_slave, TIOCSCTTY, 0);
#	endif

	/* }}} */
	/* {{{ Configure its terminal modes and window size */

	/* Set up the pty with the same termios flags as our own tty, plus  */
	/* TOSTOP, which keeps background processes from writing to the pty */

#if 0				/* Z: This can be moved to the caller */
	shell_mode.c_lflag |= TOSTOP;  /* So background writers get SIGTTOU */
	if (tcsetattr (pty_slave, TCSANOW, &shell_mode))
	{
	    perror (__FILE__": couldn't set pty terminal modes");
	    _exit (FORK_FAILURE);
	}
#endif

	/* Set the pty's size (80x25 by default on Linux) according to the */
	/* size of the real terminal as calculated by ncurses, if possible */
#if 0
#	if defined TIOCSWINSZ && !defined SCO_FLAVOR
	{
	    struct winsize tty_size;

	    tty_size.ws_row = LINES;
	    tty_size.ws_col = COLS;
	    tty_size.ws_xpixel = tty_size.ws_ypixel = 0;

	    if (ioctl (pty_slave, TIOCSWINSZ, &tty_size))
		perror (__FILE__": couldn't set pty size");
	}
#	endif
#endif

	/* }}} */
	/* {{{ Attach all our standard file descriptors to the pty */

	/* This is done just before the fork, because stderr must still	 */
	/* be connected to the real tty during the above error messages; */
	/* otherwise the user will never see them.			 */

	dup2 (pty_slave, STDIN_FILENO);
	dup2 (pty_slave, STDOUT_FILENO);
	dup2 (pty_slave, STDERR_FILENO);

	/* }}} */
	/* {{{ Execute the subshell at last */

	close (pty_slave);  /* These may be FD_CLOEXEC, but just in case... */

	/* }}} */
    } else {
      /* this is the parent process ... */
      close(pty_slave);
    }


#if 0				/* Z: this can be moved to the caller */
    /* {{{ Install our handler for SIGCHLD */

    init_sigchld ();

    /* We could have received the SIGCHLD signal for the subshell 
     * before installing the init_sigchld */
    pid = waitpid (subshell_pid, &status, WUNTRACED | WNOHANG);
    if (pid == subshell_pid){
	use_subshell = FALSE;
	return;
    }

    /* }}} */
#endif
    *master = subshell_pty;
    return subshell_pid;
}

/* }}} */
/* {{{ resize_subshell */

int resize_subshell (int fd, int col, int row, int xpixel, int ypixel)
{
#if defined TIOCSWINSZ && !defined SCO_FLAVOR
    struct winsize tty_size;

    tty_size.ws_row = row;
    tty_size.ws_col = col;
    tty_size.ws_xpixel = xpixel;
    tty_size.ws_ypixel = ypixel;

    return (ioctl (fd, TIOCSWINSZ, &tty_size));
#endif
}

/* }}} */

/* {{{ pty opening functions */

#ifdef SCO_FLAVOR

/* {{{ SCO version of pty_open_master */

static int pty_open_master (char *pty_name)
{
    int pty_master;
    int num;
    char *ptr;

    strcpy (pty_name, "/dev/ptyp");
    ptr = pty_name+9;
    for (num=0;;num++)
    {
	sprintf(ptr,"%d",num);	/* surpriiise ... SCO lacks itoa() */
	/* Try to open master */
	if ((pty_master = open (pty_name, O_RDWR)) == -1)
	    if (errno == ENOENT)  /* Different from EIO */
		return -1;	      /* Out of pty devices */
	    else
		continue;	      /* Try next pty device */
	pty_name [5] = 't';	      /* Change "pty" to "tty" */
	if (access (pty_name, 6)){
	    close (pty_master);
	    pty_name [5] = 'p';
	    continue;
	}
	return pty_master;
    }
    return -1;  /* Ran out of pty devices */
}

/* }}} */
/* {{{ SCO version of pty_open_slave */

static int pty_open_slave (const char *pty_name)
{
    int pty_slave;
    struct group *group_info = getgrnam ("terminal");
    
    if (group_info != NULL)
    {
	/* The following two calls will only succeed if we are root */
	/* [Commented out while permissions problem is investigated] */
	/* chown (pty_name, getuid (), group_info->gr_gid);  FIXME */
	/* chmod (pty_name, S_IRUSR | S_IWUSR | S_IWGRP);   FIXME */
    }
    if ((pty_slave = open (pty_name, O_RDWR)) == -1)
	perror ("open (pty_name, O_RDWR)");
    return pty_slave;
}

/* }}} */

#elif HAVE_GRANTPT

/* {{{ System V version of pty_open_master */

static int pty_open_master (char *pty_name)
{
    char *slave_name;
    int pty_master;

    strcpy (pty_name, "/dev/ptmx");
    if ((pty_master = open (pty_name, O_RDWR)) == -1
	|| grantpt (pty_master) == -1		  /* Grant access to slave */
	|| unlockpt (pty_master) == -1		  /* Clear slave's lock flag */
	|| !(slave_name = ptsname (pty_master)))  /* Get slave's name */
    {
	close (pty_master);
	return -1;
    }
    strcpy (pty_name, slave_name);
    return pty_master;
}

/* }}} */
/* {{{ System V version of pty_open_slave */

static int pty_open_slave (const char *pty_name)
{
    int pty_slave = open (pty_name, O_RDWR);

    if (pty_slave == -1)
    {
	perror ("open (pty_name, O_RDWR)");
	return -1;
    }

#if !defined(__osf__)
    if (!ioctl (pty_slave, I_FIND, "ptem"))
	if (ioctl (pty_slave, I_PUSH, "ptem") == -1)
	{
	    perror ("ioctl (pty_slave, I_PUSH, \"ptem\")");
	    close (pty_slave);
	    return -1;
	}
	
    if (!ioctl (pty_slave, I_FIND, "ldterm"))
        if (ioctl (pty_slave, I_PUSH, "ldterm") == -1)
	{
	    perror ("ioctl (pty_slave, I_PUSH, \"ldterm\")");
	    close (pty_slave);
	    return -1;
	}

#if !defined(sgi) && !defined(__sgi)
    if (!ioctl (pty_slave, I_FIND, "ttcompat"))
        if (ioctl (pty_slave, I_PUSH, "ttcompat") == -1)
	{
	    perror ("ioctl (pty_slave, I_PUSH, \"ttcompat\")");
	    close (pty_slave);
	    return -1;
	}
#endif /* sgi || __sgi */
#endif /* __osf__ */

    return pty_slave;
}

/* }}} */

#else

/* {{{ BSD version of pty_open_master */

static int pty_open_master (char *pty_name)
{
    int pty_master;
    char *ptr1, *ptr2;

    strcpy (pty_name, "/dev/ptyXX");
    for (ptr1 = "pqrstuvwxyzPQRST"; *ptr1; ++ptr1)
    {
	pty_name [8] = *ptr1;
	for (ptr2 = "0123456789abcdef"; *ptr2; ++ptr2)
	{
	    pty_name [9] = *ptr2;

	    /* Try to open master */
	    if ((pty_master = open (pty_name, O_RDWR)) == -1) {
		if (errno == ENOENT)  /* Different from EIO */
		    return -1;	      /* Out of pty devices */
		else
		    continue;	      /* Try next pty device */
	    }
	    pty_name [5] = 't';	      /* Change "pty" to "tty" */
	    if (access (pty_name, 6)){
		close (pty_master);
		pty_name [5] = 'p';
		continue;
	    }
	    return pty_master;
	}
    }
    return -1;  /* Ran out of pty devices */
}

/* }}} */
/* {{{ BSD version of pty_open_slave */

static int pty_open_slave (const char *pty_name)
{
    int pty_slave;
    struct group *group_info = getgrnam ("tty");

    if (group_info != NULL)
    {
	/* The following two calls will only succeed if we are root */
	/* [Commented out while permissions problem is investigated] */
	/* chown (pty_name, getuid (), group_info->gr_gid);  FIXME */
	/* chmod (pty_name, S_IRUSR | S_IWUSR | S_IWGRP);   FIXME */
    }
    if ((pty_slave = open (pty_name, O_RDWR)) == -1)
	perror ("open (pty_name, O_RDWR)");
    return pty_slave;
}

/* }}} */

#endif

/* }}} */

/* {{{ Emacs local variables */

/*
  Cause emacs to enter folding mode for this file:
  Local variables:
  end:
*/

/* }}} */
