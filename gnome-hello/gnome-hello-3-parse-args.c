/* gnome-hello-parse-args.c -- Example for the "parsing args" section
   of the Gnome Developers' Tutorial (that's is included in the
   Gnome Developers' Documentation in devel-progs/)
   */
/* Includes: Basic stuff
	     Menus
	     Internationalization
	     Parse of arguments
	     */

/* Copyright (C) 1998 Mark Galassi, Horacio J. Peña, all rights reserved */

/* including gnome.h gives you all you need to use the gtk toolkit as
   well as the GNOME libraries; it also handles internationalization
   via GNU gettext. Including config.h before gnome.h is very important
   (else gnome-i18n can't find ENABLE_NLS), of course i'm assuming
   that we're in the gnome tree. */
#include <config.h>
#include <gnome.h>
/* We are using GNU GetOpt */
#ifdef HAVE_GETOPT_LONG
#include <getopt.h>
#else
#include <support/getopt.h>
#endif

void hello_cb (GtkWidget *widget, void *data);
void about_cb (GtkWidget *widget, void *data);
void quit_cb (GtkWidget *widget, void *data);

void prepare_app();
void parse_args (int argc, char *argv[]);
GtkMenuFactory *create_menu ();

GtkWidget *app;

/* The menu definitions: File/Exit and Help/About are mandatory */
GtkMenuEntry hello_menu [] = {
  { N_("File/Exit"),	 N_("<control>E"), (GtkMenuCallback) quit_cb,  NULL },
	/* The '...' end indicate that the options open a dialog */
  { N_("Help/About..."), N_("<control>A"), (GtkMenuCallback) about_cb, NULL },
};

int
main(int argc, char *argv[])
{
  /* gnome_init() is always called at the beginning of a program.  it
     takes care of initializing both Gtk and GNOME */
  gnome_init (&argc, &argv);

  /* Now we parse the arguments (i would prefer having a first parsing
     before the gnome_init so we could do the --version and --help 
     without having a DISPLAY available. Tom Tromey and Carsten Schaar
     are working on it. */
  parse_args (argc, argv);

  /* Initialize the i18n stuff */
  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  textdomain (PACKAGE);

  /* prepare_app() makes all the gtk calls necessary to set up a
     minimal Gnome application; It's based on the hello world example
     from the Gtk+ tutorial */
  prepare_app ();

  gtk_main ();

  return 0;
}

void
prepare_app()
{
  GtkWidget *button;
  GtkMenuFactory *mf;

  /* Make the main window and binds the delete event so you can close
     the program from your WM */
  app = gnome_app_new ("hello", _("Hello World Gnomified") );
  gtk_widget_realize (app);
  gtk_signal_connect (GTK_OBJECT (app), "delete_event",
                      GTK_SIGNAL_FUNC (quit_cb),
                      NULL);

  /* Now that we've the main window we'll make the menues */
  /* I'm using GtkMenuFactory, i've asked to the gnome-list if i should
     use gnome_app_create_menu instead and i'm waiting the answer */
  mf = create_menu ();
  gnome_app_set_menus ( GNOME_APP (app), GTK_MENU_BAR (mf->widget));

  /* We make a button, bind the 'clicked' signal to hello and setting it
     to be the content of the main window */
  button = gtk_button_new_with_label (_("Hello GNOME"));
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
     		      GTK_SIGNAL_FUNC (hello_cb), NULL);
  gtk_container_border_width (GTK_CONTAINER (button), 60);
  gnome_app_set_contents ( GNOME_APP (app), button);

  /* We now show the widgets, the order doesn't matter, but i suggests 
     showing the main window last so the whole window will popup at
     once rather than seeing the window pop up, and then the button form
     inside of it. Although with such simple example, you'd never notice. */
  gtk_widget_show (button);
  gtk_widget_show (app);
}

/* Callbacks functions */

void
hello_cb (GtkWidget *widget, void *data)
{
  g_print (_("Hello GNOME\n"));
  gtk_main_quit ();
  return;
}

void
quit_cb (GtkWidget *widget, void *data)
{
  gtk_main_quit ();
  return;
}

void
about_cb (GtkWidget *widget, void *data)
{
  GtkWidget *about;
  gchar *authors[] = {
/* Here should be your names */
	  "Mark Galassi",
	  "Horacio J. Peña",
          NULL
          };

  about = gnome_about_new ( _("The Hello World Gnomified"), VERSION,
        		/* copyrigth notice */
                        "(C) 1998 the Free Software Foundation",
                        authors,
                        /* another comments */
                        _("GNOME is a civilized software system "
			  "so we've a \"hello world\" program"),
                        NULL);
  gtk_widget_show (about);

  return;
}

/* Menu creation */

#define ELEMENTS(x) (sizeof (x) / sizeof (x [0]))

GtkMenuFactory *
create_menu () 
{
  GtkMenuFactory *subfactory;
  int i;

  /* Internationalization */
  for (i = 0; i < ELEMENTS(hello_menu); i++)
    hello_menu[i].path = _(hello_menu[i].path);

  subfactory = gtk_menu_factory_new  (GTK_MENU_FACTORY_MENU_BAR);
  gtk_menu_factory_add_entries (subfactory, hello_menu, ELEMENTS(hello_menu));

  return subfactory;
}

/* parsing args */
void
parse_args (int argc, char *argv[])
{
  gint ch;

  struct option options[] = {
	/* Default args */
	{ "help",		no_argument,            NULL,   'h'     },
	{ "version",	 	no_argument,            NULL,   'v'     },

	{ NULL, 0, NULL, 0 }
	};

  /* initialize getopt */
  optarg = NULL;
  optind = 0;
  optopt = 0;

  while( (ch = getopt_long(argc, argv, "hv", options, NULL)) != EOF )
  {
    switch(ch)
    {
      case 'h':
        g_print ( 
      	  _("%s: A gnomified 'Hello World' program\n\n"
      	    "Usage: %s [--help] [--version]\n\n"
      	    "Options:\n"
      	    "        --help     display this help and exit\n"
      	    "        --version  output version information and exit\n"),
      	    argv[0], argv[0]);
        exit(0);
        break;
      case 'v':
        g_print (_("Gnome Hello %s.\n"), VERSION);
        exit(0);
        break;
      case ':':
      case '?':
        g_print (_("Options error\n"));
        exit(0);
        break;
    }
  }

  return;
}
