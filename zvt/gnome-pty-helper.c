/*
 * gnome-pty.c:  Helper setuid application used to open a pseudo-
 * terminal, set the permissions, ownership and record user login
 * information
 *
 * Author:
 *    Miguel de Icaza (miguel@gnu.org)
 *
 * Parent application talks to us via a couple of sockets that are strategically
 * placed on file descriptors 0 and 1 (STDIN_FILENO and STDOUT_FILENO).
 *
 * We use the STDIN_FILENO to read and write the protocol information and we use
 * the STDOUT_FILENO to pass the file descriptors (we need two different file
 * descriptors as using a socket for both data transfers and file descriptor
 * passing crashes some BSD kernels according to Theo de Raadt)
 *
 * A sample protocol is used:
 *
 * OPEN_PTY             => 1 <tag> <master-pty-fd> <slave-pty-fd>
 *                      => 0
 *
 * CLOSE_PTY  <tag>     => void
 *
 * <tag> is a pointer.  If tag is NULL, then the ptys were not allocated.
 * ptys are passed using file descriptor passing on the stdin file descriptor
 * 
 * We use as little as possible external libraries.  
 */
#include <config.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <malloc.h>
#include <termios.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <utmp.h>
#include "gnome-pty.h"
#include "gnome-login-support.h"

static struct passwd *pwent;
static char login_name_buffer [48];
static char *login_name, *display_name;

struct pty_info {
	struct pty_info *next;
	int    master_fd, slave_fd;
	char   *line;
};

typedef struct pty_info pty_info;

static pty_info *pty_list;

#ifdef HAVE_SENDMSG
#include <sys/socket.h>
#include <sys/uio.h>

#ifdef HAVE_SYS_UN_H /* Linux libc5 */
#include <sys/un.h>
#endif

#ifndef CMSG_DATA /* Linux libc5 */
/* Ancillary data object manipulation macros.  */
#if !defined __STRICT_ANSI__ && defined __GNUC__ && __GNUC__ >= 2
# define CMSG_DATA(cmsg) ((cmsg)->cmsg_data)
#else
# define CMSG_DATA(cmsg) ((unsigned char *) ((struct cmsghdr *) (cmsg) + 1))
#endif
#endif /* CMSG_DATA */

#define CONTROLLEN (sizeof (struct cmsghdr)  + sizeof (int))

static struct cmsghdr *cmptr;

static int
init_msg_pass ()
{
	cmptr = malloc (CONTROLLEN);

	if (cmptr)
		return 0;

	return -1;
}

static int
pass_fd (int client_fd, int fd)
{
        struct iovec  iov[1];
        struct msghdr msg;
        char          buf [1];

	iov [0].iov_base = buf;
	iov [0].iov_len  = 1;

	msg.msg_iov        = iov;
	msg.msg_iovlen     = 1;
	msg.msg_name       = NULL;
	msg.msg_namelen    = 0;
	msg.msg_control    = cmptr;
	msg.msg_controllen = CONTROLLEN;
		
	cmptr->cmsg_level = SOL_SOCKET;
	cmptr->cmsg_type  = SCM_RIGHTS;
	cmptr->cmsg_len   = CONTROLLEN;
	*(int *)CMSG_DATA (cmptr) = fd;

	if (sendmsg (client_fd, &msg, 0) != 1)
		return -1;

	return 0;
}
#else
#include <stropts.h>
static int
init_msg_pass ()
{
	/* nothing */
}

int
pass_fd (int client_fd, int fd)
{
	if (ioctl (client_fd, I_SENDFD, fd) < 0)
		return -1;
	return 0;
}
#endif

static void
pty_free (pty_info *pi)
{
	free (pi);
}

static void
pty_remove (pty_info *pi)
{
	pty_info *l, *last;

	last = (void *) 0;
	
	for (l = pty_list; l; l = pi->next){
		if (l == pi){
			if (last == (void *) 0)
				pty_list = pi->next;
			else
				last->next = pi->next;
			free (pi->line);
			pty_free (pi);
			break;
		}
	}

	exit (1);
}

static void
shutdown_pty (pty_info *pi)
{
#ifdef HAVE_LOGIN
	logwtmp (pi->line, "", "");
	logout (pi->line);
#endif
	close (pi->master_fd);
	close (pi->slave_fd);

	pty_remove (pi);
}

static void
shutdown_helper (void)
{
	pty_info *pi;
	
	for (pi = pty_list; pi; pi = pty_list)
		shutdown_pty (pi);
}

static pty_info *
pty_add (int master_fd, int slave_fd, char *line)
{
	pty_info *pi = malloc (sizeof (pty_info));

	if (pi == NULL){
		shutdown_helper ();
		exit (1);
	}

	if (strncmp (line, "/dev/", 5))
		pi->line = strdup (line);
	else
		pi->line = strdup (line+5);
	
	pi->master_fd = master_fd;
	pi->slave_fd  = slave_fd;
	pi->next = pty_list;

	pty_list = pi;

	return pi;
}

static int
open_ptys (int update_db)
{
	char *term_name;
	int status, master_pty, slave_pty;
	pty_info *p;
	int result;

#ifdef _SC_TTY_NAME_MAX
	term_name = alloca (sysconf (_SC_TTY_NAME_MAX) + 1);
#else
	term_name = alloca (PATH_MAX) + 1;
#endif
	
	status = openpty (&master_pty, &slave_pty, term_name, NULL, NULL);
	if (status == -1){
		result = 0;
		write (STDIN_FILENO, &result, sizeof (result));
		return 0;
	}

	p = pty_add (master_pty, slave_pty, term_name);
	result = 1;

	if (write (STDIN_FILENO, &result, sizeof (result)) == -1 ||
	    write (STDIN_FILENO, &p, sizeof (p)) == -1 ||
	    pass_fd (STDOUT_FILENO, master_pty)  == -1 ||
	    pass_fd (STDOUT_FILENO, slave_pty)   == -1){
		exit (0);
	}

	if (update_db){
#ifdef HAVE_LOGIN
		/* We have to fork and make the slave_pty our
		 * controlling pty if we don't want to clobber the
		 * parent's one ...
		 */
		pid_t pid;
		
		pid = fork ();
		if (pid == 0) {
			setsid ();
			while (dup2 (slave_pty, 0) == -1 && errno == EINTR)
				;
#ifdef USE_SYSV_UTMP
			/* [FIXME]: This is only a hack since we
			 * declared `update_dbs' conditionally to
			 * the same #ifdef in gnome-utmp.c.
			 */
			update_dbs (login_name, display_name, term_name);
#endif
			exit (0);
		}
		
		return 1;
#else
#ifdef USE_SYSV_UTMP
		/* [FIXME]: This is only a hack since we declared
		 * `update_dbs' conditionally to the same #ifdef
		 * in gnome-utmp.c.
		 */
		update_dbs (login_name, display_name, term_name);
#endif
#endif
	}
	
	return 1;
}

static void
close_pty_pair (void *tag)
{
	pty_info *pi;

	for (pi = pty_list; pi; pi = pi->next){
		if (tag == pi){
			shutdown_pty (pi);
			break;
		}
	}
}

int
main (int argc, char *argv [])
{
	int res;
	void *tag;
	GnomePtyOps op;
	int stderr_fd;
	
	/*
	 * File descriptors 0 and 1 have been setup by the parent process
	 * to be used for the protocol exchange and for transfering
	 * file descriptors.
	 *
	 * Make stderr point to a terminal.
	 */
	close (2);
	stderr_fd = open ("/dev/tty", O_RDWR);
	if (stderr_fd != 2){
		while (dup2 (stderr_fd, 2) == -1 && errno == EINTR)
			;
	}
	pwent = getpwuid (getuid ());
	if (pwent)
		login_name = pwent->pw_name;
	else {
		sprintf (login_name_buffer, "#%d", getuid ());
		login_name = login_name_buffer;
	}

	display_name = getenv ("DISPLAY");
	if (!display_name)
		display_name = "localhost";
	
	
	if (init_msg_pass () == -1)
		exit (1);

	for (;;){
		res = read (STDIN_FILENO, &op, sizeof (op));
		
		if (res == -1){
			if (errno == EINTR)
				continue;

			shutdown_helper ();
			return 1;
		}

		if (res == 0) {
			shutdown_helper ();
			return 1;
		}

		switch (op){
		case GNOME_PTY_OPEN_PTY:
			open_ptys (1);
			break;
			
		case GNOME_PTY_OPEN_NO_DB_UPDATE:
			open_ptys (0);
			break;
			
		case GNOME_PTY_CLOSE_PTY:
			if (read (STDIN_FILENO, &tag, sizeof (tag)) == -1){
				shutdown_helper ();
				return 1;
			}
			close_pty_pair (tag);
			break;
		}
		
	}

	return 0;
}


