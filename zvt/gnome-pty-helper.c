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
 * OPEN_PTY             => <tag> <master-pty-fd> <slave-pty-fd>
 * CLOSE_PTY  <tag>     => void
 *
 * <tag> is a pointer.
 *
 * We use as little as possible external libraries.  No stdio, no glib.
 */
#include <config.h>
#include <unistd.h>
#include <errno.h>
#include "gnome-pty.h"

static char *err1 = "pty_remove: can not find pty_info, this should not happen\n";

struct pty_info {
	struct PTYList *next;
	int    master_fd, slave_fd;
};

typedef struct pty_info pty_info;

static pty_info *pty_list;

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
			free_pty (pi);
			break;
		}
	}

	write (STDERR_FILENO, err1, sizeof (err1));
	exit (1);
}

static void
shutdown_pty (pty_info *pi)
{
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

static void *
pty_add (int master_fd, int slave_fd)
{
	pty_info *pi = malloc (sizeof (pty_info));

	if (pi == NULL){
		shutdown_helper ();
		exit (1);
	}

	pi->master_fd = master_fd;
	pi->slave_fd  = slave_fd;
	pi->next = pty_list;

	pty_list = pi;

	return pi;
}

void
open_ptys ()
{
	
}

void
update_dbs ()
{
}

int
main (int argc, char *argv [])
{
	int res, tag;
	GnomePtyOps op;
	
	for (;;){
		res = read (STDIN_FILENO, &op, sizeof (op));
		
		if (res == -1){
			if (errno == EINTR)
				continue;

			shutdown_helper ();
			return 1;
		}

		switch (op){
		case GNOME_PTY_OPEN_PTY:
			open_ptys ();
			update_dbs ();
			break;
			
		case GNOME_PTY_OPEN_NO_DB_UPDATE:
			open_ptys ();
			
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


