/*
 * Subprocess activation under a pseudo terminal support for zvtterm
 *
 * Copyright (C) 1994, 1995, 1998 Dugan Porter
 * Copyright (C) 1998 Michael Zucchi
 * Copyrithg (C) 1995, 1998 Miguel de Icaza
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of Version 2 of the GNU Library General Public
 * License, as published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <config.h>

#include <sys/types.h>

#include "subshell-includes.h"

/* Pid of the helper SUID process */
static pid_t helper_pid;

/* Whether sigchld signal handler has been established yet */
static int sigchld_inited = 0;

/* Points to a possibly previously installed sigchld handler */
static struct sigaction old_sigchld_handler;

/* The socketpair used for the protocol */
int helper_socket_protocol  [2];

/* The paralell socketpair used to transfer file descriptors */
int helper_socket_fdpassing [2];

/* List of all subshells we're watching */
struct child_list {
	pid_t pid;
	int fd;
	struct child_list * next;
} * children = NULL;

void
zvt_close_msgfd (pid_t childpid)
{
	struct child_list * prev, * child;

	child = children;
	prev = NULL;

	while (child && child->pid != childpid){
		prev = child;
		child = child->next;
	}
	
	if (!child) return;
	
	if (!prev)
		children = child->next;
	else
		prev->next = child->next;
	
	close (child->fd);
	g_free (child);
}

/*
 *  Fork the subshell, and set up many, many things.
 *
 */

static void
sigchld_handler (int signo)
{
	struct child_list * child;
	pid_t pid;
	int status;

	if (waitpid (helper_pid, &status, WNOHANG) == helper_pid){
		helper_pid = 0;
		return;
	}

	for (child = children; child; child = child->next){
		if (waitpid (child->pid, &status, WNOHANG) == child->pid){
			child->pid = 0;
			write (child->fd, "D", 1);
			return;
		}
	}

	/* No children of ours, chain */
	if (old_sigchld_handler.sa_handler)
		(*old_sigchld_handler.sa_handler)(signo);
}

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

static struct cmsghdr *cmptr;
#define CONTROLLEN  sizeof (struct cmsghdr) + sizeof (int)

static int
receive_fd (int helper_fd)
{
	struct iovec iov [1];
	struct msghdr msg;
	char buf [32];
	
	iov [0].iov_base = buf;
	iov [0].iov_len  = sizeof (buf);
	msg.msg_iov      = iov;
	msg.msg_iovlen   = 1;
	msg.msg_name     = NULL;
	msg.msg_namelen  = 0;

	if (cmptr == NULL && (cmptr = g_malloc (CONTROLLEN)) == NULL)
		return -1;
	msg.msg_control = cmptr;
	msg.msg_controllen = CONTROLLEN;

	if (recvmsg (helper_fd, &msg, 0) <= 0)
		return -1;

	return *(int *) CMSG_DATA (cmptr);
}
#else
static int
receive_fd (int helper_fd)
{
	int flag;
	char buf [128];
	struct strbuf dat;
	struct strrecvfd recvfd;
	
	dat.buf = buf;
	dat.maxlen = sizeof (buf);
	flag = 0;
	
	if (getmsg (helper_fd, NULL, &dat, &flag) < 0)
		return -1;

	if (dat.len == 0)
		return -1;

	if (ioctl (helper_fd, I_RECVFD, &recvfd) < 0)
		return -1;

	return recvfd.fd;
}

#endif

static void *
get_ptys (int *master, int *slave, int update_wutmp)
{
	GnomePtyOps op;
	int result, n;
	void *tag;
	
	if (helper_pid == -1)
		return NULL;

	if (helper_pid == 0){
		if (socketpair (AF_UNIX, SOCK_STREAM, 0, helper_socket_protocol) == -1)
			return NULL;

		if (socketpair (AF_UNIX, SOCK_STREAM, 0, helper_socket_fdpassing) == -1){
			close (helper_socket_protocol [0]);
			close (helper_socket_protocol [1]);
			return NULL;
		}
		
		helper_pid = fork ();
		
		if (helper_pid == -1){
			close (helper_socket_protocol [0]);
			close (helper_socket_protocol [1]);
			close (helper_socket_fdpassing [0]);
			close (helper_socket_fdpassing [1]);
			return NULL;
		}

		if (helper_pid == 0){
			close (0);
			close (1);
			dup2 (helper_socket_protocol  [1], 0);
			dup2 (helper_socket_fdpassing [1], 1);

			/* Close aliases */
			close (helper_socket_protocol  [0]);
			close (helper_socket_protocol  [1]);
			close (helper_socket_fdpassing [0]);
			close (helper_socket_fdpassing [1]);

			execl (GNOMESBINDIR "/gnome-pty-helper", "gnome-pty-helper", NULL);
			exit (1);
		} else {
			close (helper_socket_fdpassing [1]);
			close (helper_socket_protocol  [1]);
		}
	}
	if (update_wutmp)
		op = GNOME_PTY_OPEN_PTY;
	else
		op = GNOME_PTY_OPEN_NO_DB_UPDATE;
	
	if (write (helper_socket_protocol [0], &op, sizeof (op)) < 0)
		return NULL;
	
	n = n_read (helper_socket_protocol [0], &result, sizeof (result));
	if (n == -1 || n != sizeof (result)){
		helper_pid = 0;
		return NULL;
	}
	
	if (result == 0)
		return NULL;

	n = n_read (helper_socket_protocol [0], &tag, sizeof (tag));
	
	if (n == -1 || n != sizeof (tag)){
		helper_pid = 0;
		return NULL;
	}

	*master = receive_fd (helper_socket_fdpassing [0]);
	*slave  = receive_fd (helper_socket_fdpassing [0]);
	
	return tag;
}

/**
 * zvt_init_subshell:
 * @vt:       the terminal emulator object.
 * @pty_name: Name of the pseudo terminal opened.
 * @log:      if TRUE, then utmp/wtmp records are updated
 *
 * Returns the pid of the subprocess.
 */
int
zvt_init_subshell (struct vt_em *vt, char *pty_name, int log)
{
	int slave_pty, master_pty;
	int subshell_pid;
	struct sigaction sa;
	struct child_list * child;
	pid_t pid;
	int status;
	int p[2];

	g_return_val_if_fail (vt != NULL, -1);
	
	if (!sigchld_inited){
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = sigchld_handler;
		sigaction (SIGCHLD, &sa, &old_sigchld_handler);
		sigchld_inited = 1;
	}

	if ((vt->pty_tag = get_ptys (&master_pty, &slave_pty, log)) == NULL)
		return -1;

	/* Fork the subshell */

	vt->childpid = fork ();
	
	if (vt->childpid == -1)
		return -1;

	if (vt->childpid == 0)  /* We are in the child process */
	{
		close (master_pty);
		login_tty (slave_pty);
	} else {
		close (slave_pty);
	}
	
	pipe(p);
	vt->msgfd = p [0];
	
	child = g_malloc(sizeof(*child));
	child->next = children;
	child->pid = vt->childpid;
	child->fd = p[1];
	children = child;
	
	/* We could have received the SIGCHLD signal for the subshell 
	 * before installing the init_sigchld
	 */
	pid = waitpid (vt->childpid, &status, WUNTRACED | WNOHANG);
	if (pid == vt->childpid && child->pid >= 0){
		child->pid = 0;
		write (child->fd, "D", 1);
		return -1;
	}
	
	vt->keyfd = vt->childfd = master_pty;

	return vt->childpid;
}

int zvt_resize_subshell (int fd, int col, int row, int xpixel, int ypixel)
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

/**
 * zvt_shutdown_subsheel:
 * @vt: The terminal emulator object
 * 
 * Shuts down the pseudo terminal information. 
 **/
void
zvt_shutdown_subshell (struct vt_em *vt)
{
	GnomePtyOps op;
	
	g_return_if_fail (vt != NULL);

	if (vt->pty_tag == NULL)
		return;

	op = GNOME_PTY_CLOSE_PTY;
	write (helper_socket_protocol [0], &op, sizeof (op));
	write (helper_socket_protocol [0], &vt->pty_tag, sizeof (vt->pty_tag));
	vt->pty_tag == NULL;
}
