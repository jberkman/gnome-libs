/*
 * gnome-login-support.c: Replacement for systems that lack login_tty, open_pty and forkpty
 *
 * Author:
 *    Miguel de Icaza (miguel@gnu.org)
 *
 *
 */
#include <config.h>
#include <unistd.h>
#include <sys/ioctl.h>

#ifndef HAVE_LOGIN_TTY
int
login_tty (int fd)
{
	/* Create the session */
	setsid ();

#ifdef TIOCSCTTY
	if (ioctl (fd, TIOCSCTTY, 0) == -1)
		return -1;
#endif

	dup2 (fd, 0);
	dup2 (fd, 1);
	dup2 (fd, 2);
	if (fd > 2)
		close (fd);

	return 0;
}
#endif

#ifndef HAVE_OPENPTY

static int
pty_open_master (char *pty_name)
{
	strcpy (pty_name, "/dev/ptmx");

	pty_master = open (pty_name, O_RDWR);

	/* Try BSD open */
	if (pty_master == -1)
		return pty_open_master_bsd (pty_name);

#ifdef HAVE_
}

int
open_pty (int *master_fd, int *slavefd, char *name, struct termios *termp, struct winsize *winp)
{
	int pty_master, pty_slave;
	char line [256];
	
	pty_master = pty_open_master (line);

	if (pty_master == -1)
		return -1;

	chown (line, getuid (), ttygid);
	chmod (line, S_IRUSR|S_IWUSR|S_IWGRP);
#ifdef HAVE_REVOKE
	revoke (line);
#endif
	
	/* Open slave side */
	slave = pty_open_slave (line);
	if (pty_slave == -1){
		close (pty_master);

		errno = ENOENT;
		return -1;
	}

	*master_fd = pty_master;
	*slave_fd  = pty_slave;

	if (termp)
		tcsetattr (slave_fd, TCSAFLUSH, termp);

	if (winp)
		ioctl (slave_fd, TIOCSWINSZ, winp);
	
	if (name)
		strcpy (name, line);

	return 0;
}
#endif

#ifndef HAVE_FORKPTY
pid_t
forkpty (int *master_fd, char *name, struct termios *termp, struct winsize *winp)
{
	int master, slave;
	
	if (openpty (&master, &slave, name, termp, winp) == -1)
		return -1;

	pid = fork ();

	if (pid == -1)
		return -1;

	/* Child */
	if (pid == 0){
		close (master);
		login_tty (slave);
	} else {
		*master_fd = master;
		close (slave);
	}
	
	return pid;
}
#endif
