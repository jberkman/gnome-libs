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
#include "subshell-includes.h"

/* Pid of the helper SUID process */
static pid_t helper_pid;

int helper_socket [2];

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
	free (child);
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

	pid = wait (&status);

	if (pid == helper_pid){
		helper_pid = 0;
		return;
	}
	
	child = children;
	while (child && child->pid != pid) {
		child = child->next;
	}
	
	if (child) {
		child->pid = 0;
		write(child->fd, "D", 1);
	}
}

#ifdef HAVE_SENDMSG
#include <sys/socket.h>
#include <sys/uio.h>

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

	if (cmptr == NULL && (cmptr = malloc (CONTROLLEN)) == NULL)
		return -1;
	msg.msg_control = cmptr;
	msg.msg_controllen = CONTROLLEN;

	if (recvmsg (helper_fd, &msg, 0) <= 0)
		return -1;

	return *(int *) CMSG_DATA (cmptr);
}
#else
static mint
receive_fd (int helper_fd)
{
	int flag;
	struct buf [128];
	struct strbuf dat;
	struct strrecvfd recvfd;
	
	dat.buf = buf;
	dat.maxlen = sizeof (buf);
	flag = 0;
	
	if (getmsg (helper_fd, NULL, &dat, &flag) < 0)
		return -1;

	if (dat.len == 0)
		return -1;

	if (ioctl (helper_fd, I_RECVFD, &fd) < 0)
		return -1;

	return recvfd.fd;
}

#endif

static void *
get_ptys (int *master, int *slave, int update_wutmp)
{
	GnomePtyOps op;
	int result;
	void *tag;
	
	if (helper_pid == -1)
		return NULL;

	if (helper_pid == 0){
		if (socketpair (AF_UNIX, SOCK_STREAM, 0, helper_socket) == -1)
			return NULL;
		
		helper_pid = fork ();
		
		if (helper_pid == -1)
			return NULL;

		if (helper_pid == 0){
			close (0);
			close (1);
			dup2 (helper_socket [1], 0);
			dup2 (helper_socket [1], 1);

			/* Close aliases */
			close (helper_socket [0]);
			close (helper_socket [1]);

			execl (GNOMESBINDIR "/gnome-pty-helper", "gnome-pty-helper", NULL);
			exit (1);
		} else {
			close (helper_socket [1]);
		}
	}
	if (update_wutmp)
		op = GNOME_PTY_OPEN_PTY;
	else
		op = GNOME_PTY_OPEN_NO_DB_UPDATE;
	
	if (write (helper_socket [0], &op, sizeof (op)) < 0)
		return NULL;
	
	read (helper_socket [0], &result, sizeof (result));
	if (result == 0)
		return NULL;

	read (helper_socket [0], &tag, sizeof (tag));

	*master = receive_fd (helper_socket [0]);
	*slave  = receive_fd (helper_socket [0]);
	
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
	
	if (get_ptys (&master_pty, &slave_pty, log) == NULL)
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
	
	if (!children) {
		memset(&sa, 0, sizeof(sa));
		sa.sa_handler = sigchld_handler;
		sigaction(SIGCHLD, &sa, NULL);
	} 
	
	pipe(p);
	vt->msgfd = p [0];
	
	child = malloc(sizeof(*child));
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
	
	vt->childfd = master_pty;

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

