/* gnome-hello.c -- this "hello world" style program is meant to be a
   "gnominally correct" C program */

/* Copyright (C) 1998 Mark Galassi, all rights reserved */

/* including gnome.h gives you all you need to use the gtk toolkit as
   well as the GNOME libraries; it also handles internationalization
   via GNU gettext */
#include <gnome.h>

/* the following are needed for internationalization using GNU gettext */
/* #include <libintl.h> */
/* #define _(String) (String) */
/* #define N_(String) (String) */
/* #define textdomain(Domain) */
/* #define bindtextdomain(Package, Directory) */

void prepare_app(int argc, char *argv[]);

int main(int argc, char *argv[])
{
  /* gnome_init() is always called at the beginning of a program.  it
     takes care of initializing both Gtk and GNOME */
  gnome_init(&argc, &argv);

  /* prepare_app() makes all the gtk calls necessary to set up a
     minimal application; in fact, I copied most of it from the Gtk
     manual. */
  prepare_app(argc, argv);

  gtk_main();

  return 0;
}

void
hello (void)
{
  g_print (_("Hello GNOME\n"));
  gtk_exit (0);
}

void prepare_app(int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *button;

  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_container_border_width (GTK_CONTAINER (window), 10);

  button = gtk_button_new_with_label (_("Hello GNOME"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
     		      GTK_SIGNAL_FUNC (hello), NULL);
  gtk_container_add (GTK_CONTAINER (window), button);
  gtk_widget_show (button);

  gtk_widget_show (window);

  gtk_main ();
}
