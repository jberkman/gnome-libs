/* gnome-exec.c - Execute some command.

   Copyright (C) 1998 Tom Tromey

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef GNOME_EXEC_H
#define GNOME_EXEC_H

BEGIN_GNOME_DECLS


/* Fork and execute some program in the background.  Returns -1 on
   error.  Returns PID on success.  Should correctly report errno
   returns from a failing child invocation.  DIR is the directory in
   which to exec the child; if NULL the current directory is used.
   Searches $PATH to find the child.  */
int gnome_execute_async (const char *dir, int argc, char * const argv[]);

/* Like gnome_execute_async, but each string in ENVV is added to the
   child's environment.  If you want to set the environment exactly,
   you must set the global `environ' variable instead.  If ENVV is
   NULL, the child inherits the parent's environment.  In this case,
   the value of ENVC is ignored.  */
int gnome_execute_async_with_env (const char *dir,
				  int argc, char * const argv[],
				  int envc, char * const envv[]);

/* Fork and execute commandline using the user's shell. Calls 
   gnome_execute_async so it does the same things and returns 
   the same things. */
int gnome_execute_shell (const char *dir, const char *commandline);

END_GNOME_DECLS

#endif /* GNOME_EXEC_H */
