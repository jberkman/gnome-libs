#ifndef _SUBSHELL_H
#define _SUBSHELL_H

/* Concurrent shell support for the Midnight Commander
   Copyright (C) 1994, 1995 Dugan Porter

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

int init_subshell (int *master, char *pty_name, int * msgfd);
int resize_subshell (int fd, int col, int row, int xpixel, int ypixel);
void close_msgfd (pid_t childpid);

#endif /* _SUBSHELL_H */
