/*
 * gnome-pty.c:  Helper setuid application used to open a pseudo-
 * terminal, set the permissions, ownership and record user login
 * information
 *
 * Author:
 *    Miguel de Icaza (miguel@gnu.org)
 *
 * Parent application talks to us via a pipe, sample protocol is used:
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
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <termios.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <utmp.h>
#include "gnome-pty.h"
#include "gnome-login-support.h"

static struct passwd *pwent;
static char *login_name, *display_name;

struct pty_info {
	struct pty_info *next;
	int    master_fd, slave_fd;
	char   line [256];
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
			pty_free (pi);
			break;
		}
	}

	exit (1);
}

static void
shutdown_pty (pty_info *pi)
{
#ifdef __FreeBSD__
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

	strncpy (pi->line, strncmp (line, "/dev/", 5) ? line : line+5, 255);
	pi->line [255] = 0;
	
	pi->master_fd = master_fd;
	pi->slave_fd  = slave_fd;
	pi->next = pty_list;

	pty_list = pi;

	return pi;
}

static int
open_ptys (int update_db)
{
	char term_name [256];
	int status, master_pty, slave_pty;
	pty_info *p;
	int result;
	
	status = openpty (&master_pty, &slave_pty, term_name, NULL, NULL);
	if (status == -1){
		result = 0;
		write (STDOUT_FILENO, &result, sizeof (result));
		return 0;
	}

	p = pty_add (master_pty, slave_pty, term_name);
	result = 1;

	write (STDOUT_FILENO, &result, sizeof (result));
	write (STDOUT_FILENO, &p, sizeof (p));
	pass_fd (STDOUT_FILENO, master_pty);
	pass_fd (STDOUT_FILENO, slave_pty);

	if (update_db){
#ifdef __FreeBSD__
		/* We have to fork and make the slave_pty our
		 * controlling pty if we don't want to clobber the
		 * parent's one ... */
		if (fork () == 0) {
			setsid ();
			dup2 (slave_pty, 0);
			update_dbs (login_name, display_name, term_name);
			exit (0);
		}
		return 1;
#else
		update_dbs (login_name, display_name, term_name);
#endif
	}
	
	return 1;
}

static void
close_pty_pair (void *tag)
{
}

int
main (int argc, char *argv [])
{
	int res;
	void *tag;
	GnomePtyOps op;

	pwent = getpwuid (getuid ());
	login_name = pwent ? pwent->pw_name : "noname";

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


