/*  zterm.c - Zed's Virtual Terminal
 *  Copyright (C) 1998  Michael Zucchi
 *
 *  A simple terminal program, based on ZTerm.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <pwd.h>

#include "zvtterm.h"

/*
  main routine

  Does setup, initialises windows, forks child.
*/
gint main (gint argc, gchar *argv[])
{
  int c;
  int cmdindex;
  int scrollbacklines;
  struct passwd *pw;
  GtkWindow *window;
  ZvtTerm *term;

  gtk_init(&argc, &argv);

  /* process arguments */
  cmdindex = 0;
  while ( (cmdindex==0) && (c=getopt(argc, argv, "e:s:")) != EOF ) {
    switch(c) {
    case 'e':
      cmdindex = optind-1;	/* index of argv array to pass to exec */
      break;
    }
  }


  /* Create toplevel window, set title and policies */
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW(window), "ZTerm");

  term = zvt_term_new ();
  gtk_container_add (GTK_CONTAINER (window), term);

  gtk_widget_show (term);
  gtk_widget_show (window);


  switch (zvt_term_forkpty(term)) {
  case -1:
    perror("ERROR: unable to fork:");
    exit(1);
    break;
  case 0:
    if (cmdindex) {
      execvp(argv[cmdindex], &argv[cmdindex]);
    } else {
				/* get shell from passwd */
      pw = getpwuid(getpid());
      if (pw) {
	execl(pw->pw_shell, rindex(pw->pw_shell, '/'), 0);
      } else {
	execl("/bin/bash", "bash", 0);
      }
    }
    perror("ERROR: Cannot exec command:");
    exit(1);
  default:
  }

  /* main loop */
  gtk_main ();

  /* should never be called - but just in case */
  gtk_exit(0);
  return 0;
}


