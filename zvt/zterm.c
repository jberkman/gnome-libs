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
#include <stdlib.h>

#include "zvtterm.h"

static void child_died_event(ZvtTerm *term)
{
  exit(0);
}

/*
  main routine

  Does setup, initialises windows, forks child.
*/
gint main (gint argc, gchar *argv[])
{
  int c;
  int cmdindex;
  int login_shell = 0;
  int scrollbacklines;
  struct passwd *pw;
  GtkWindow *window;
  ZvtTerm *term;
  GtkWidget *table;
  GtkWidget *scrollbar;

  gtk_init(&argc, &argv);

  /* process arguments */
  cmdindex = 0;
  scrollbacklines = 50;
  while ( (cmdindex==0) && (c=getopt(argc, argv, "le:s:")) != EOF ) {
    switch(c) {
    case 'e':
      cmdindex = optind-1;	/* index of argv array to pass to exec */
      break;
    case 's':
      scrollbacklines = atoi(optarg);
      break;
    case 'l':
      login_shell = 1;
      break;
    }
  }

  /* Create widgets and set options */
  window = GTK_WINDOW(gtk_window_new (GTK_WINDOW_TOPLEVEL));
  gtk_window_set_title (GTK_WINDOW(window), "ZTerm");
  gtk_window_set_policy (GTK_WINDOW(window), FALSE, TRUE, TRUE);
  table = gtk_table_new (1, 2, FALSE);

  term = ZVT_TERM(zvt_term_new ());

  zvt_term_set_scrollback(term, scrollbacklines);
  zvt_term_set_font_name(term, "-misc-fixed-medium-r-normal--20-200-75-75-c-100-iso8859-1");

  gtk_signal_connect (GTK_OBJECT (term), "child_died",
                      (GtkSignalFunc) child_died_event, NULL);

  scrollbar = gtk_vscrollbar_new (GTK_ADJUSTMENT (term->adjustment));
  GTK_WIDGET_UNSET_FLAGS (scrollbar, GTK_CAN_FOCUS);

  /* layout the widgets */
  gtk_container_add (GTK_CONTAINER (window), table);
  gtk_table_attach (GTK_TABLE (table), scrollbar, 0, 1, 0, 1,
		    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  gtk_table_attach (GTK_TABLE (table), GTK_WIDGET(term), 1, 2, 0, 1,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);


  gtk_widget_show (GTK_WIDGET(term));
  gtk_widget_show (scrollbar);
  gtk_widget_show (table);
  gtk_widget_show (GTK_WIDGET(window));

  gdk_window_set_hints (((GtkWidget *)window)->window, 0, 0, 50, 50, 0, 0, GDK_HINT_MIN_SIZE);

  switch (zvt_term_forkpty(term, TRUE)) {
  case -1:
    perror("ERROR: unable to fork:");
    exit(1);
    break;
  case 0:
    if (cmdindex) {
      execvp(argv[cmdindex], &argv[cmdindex]);
    } else {
      char basename [BUFSIZ];

				/* get shell from passwd */
      pw = getpwuid(getuid());
      if (login_shell) {
	if (pw) {
	  chdir(pw->pw_dir);
	  snprintf(basename, BUFSIZ, "-%s", rindex(pw->pw_shell, '/'));
	  execl(pw->pw_shell, basename, NULL);
	} else {
	  execl("/bin/bash", "-bash", NULL);
	}
      } else {
	if (pw) {
	  execl(pw->pw_shell, rindex(pw->pw_shell, '/'), NULL);
	} else {
	  execl("/bin/bash", "bash", NULL);
	}
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


