/* gnome-hello.c -- this "hello world" style program is meant to be a
   "gnominally correct" C program. It's the example for "Finally,
   Session Management" */
/* Includes: Basic stuff
	     Menus
	     Internationalization
	     Parse of arguments
	     Session Management
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

static int save_state (GnomeClient        *client,
		       gint                phase,
		       GnomeRestartStyle   save_style,
		       gint                shutdown,
		       GnomeInteractStyle  interact_style,
		       gint                fast,
		       gpointer            client_data);
static void connect   (GnomeClient *client, 
		       gint         was_restarted, 
		       gpointer     client_data);

void discard_session (gchar *id);

GtkWidget *app;

int restarted = 0;
/* Old Session state (it shouldn't be global variables, but i couldn't find
   a simple better way) */
int os_x = 0,
    os_y = 0,
    os_w = 0,
    os_h = 0;

/* The menu definitions: File/Exit and Help/About are mandatory */
GtkMenuEntry hello_menu [] = {
  { N_("File/Exit"),	 N_("<control>E"), (GtkMenuCallback) quit_cb,  NULL },
	/* The '...' end indicate that the options open a dialog */
  { N_("Help/About..."), N_("<control>A"), (GtkMenuCallback) about_cb, NULL },
};

char *session_id;

int
main(int argc, char *argv[])
{
  /* gnome_init() is always called at the beginning of a program.  it
     takes care of initializing both Gtk and GNOME */
  gnome_init ("gnome-hello-4-SM", &argc, &argv);

  /* Now we parse the arguments (i would prefer having a first parsing
     before the gnome_init so we could do the --version, --help and
     --discard-session without having a DISPLAY available. Tom Tromey
     and Carsten Schaar */
  /* parse_args initializes the session management stuff too. */
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

  if (restarted) {
    gtk_widget_set_uposition (app, os_x, os_y);
    gtk_widget_set_usize     (app, os_w, os_h);
    }

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
  GnomeClient *client;
  gint ch;

  struct option options[] = {
	/* Default args */
	{ "help",		no_argument,            NULL,   'h'     },
	{ "version",	 	no_argument,            NULL,   'v'     },
	/* Session Management stuff */
	{ "sm-client-id",	required_argument,      NULL,   'S'     },
	{ "discard-session",	required_argument,      NULL,   'D'     },

	{ NULL, 0, NULL, 0 }
	};

  gchar *id = NULL;

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
      case 'S':
	/* This option is only a dummy. It is evaluated from the
	   'gnome_client_new_without_connection' call.

        id = g_strdup (optarg);
        restart_session (id); */
        break;
      case 'D':
        id = g_strdup (optarg);
        discard_session (id);
        exit(0);
        break;
      case ':':
      case '?':
        g_print (_("Options error\n"));
        exit(0);
        break;
    }
  }

  /* SM stuff */
  client = gnome_client_new_without_connection (argc, argv);
  
  gtk_signal_connect (GTK_OBJECT (client), "save_yourself",
		      GTK_SIGNAL_FUNC (save_state), (gpointer) argv[0]);
  gtk_signal_connect (GTK_OBJECT (client), "connect",
		      GTK_SIGNAL_FUNC (connect), NULL);

  gnome_client_connect (client);

  g_free(id);

  return;
}

/* Session management */

static int
save_state (GnomeClient        *client,
	    gint                phase,
	    GnomeRestartStyle   save_style,
	    gint                shutdown,
	    GnomeInteractStyle  interact_style,
	    gint                fast,
	    gpointer            client_data)
{
  gchar *session_id;
  gchar *sess;
  gchar *buf;
  gchar *argv[3];
  gint x, y, w, h;

  session_id= gnome_client_get_id (client);

  /* The only state that gnome-hello has is the window geometry. 
     Get it. */
  gdk_window_get_geometry (app->window, &x, &y, &w, &h, NULL);

  /* Save the state using gnome-config stuff. */
  sess = g_copy_strings ("/gnome-hello/Saved-Session-",
                         session_id,
                         NULL);

  buf = g_copy_strings ( sess, "/x", NULL);
  gnome_config_set_int (buf, x);
  g_free(buf);
  buf = g_copy_strings ( sess, "/y", NULL);
  gnome_config_set_int (buf, y);
  g_free(buf);
  buf = g_copy_strings ( sess, "/w", NULL);
  gnome_config_set_int (buf, w);
  g_free(buf);
  buf = g_copy_strings ( sess, "/h", NULL);
  gnome_config_set_int (buf, h);
  g_free(buf);

  gnome_config_sync();
  g_free(sess);

  /* Here is the real SM code. We set the argv to the parameters needed
     to restart/discard the session that we've just saved and call
     the gnome_session_set_*_command to tell the session manager it. */
  argv[0] = (char*) client_data;
  argv[1] = "--discard-session";
  argv[2] = session_id;
  gnome_client_set_discard_command (client, 3, argv);

  return TRUE;
}

/* Connected to session manager. If restarted from a former session:
   reads the state of the previous session. Sets os_* (prepare_app
   uses them) */
void
connect (GnomeClient *client, gint was_restarted, gpointer client_data)
{
  if (was_restarted)
    {
      gchar *session_id;
      gchar *sess;
      gchar *buf;
      
      restarted = 1;
      
      session_id= gnome_client_get_id (client);
      
      sess = g_copy_strings ("/gnome-hello/Saved-Session-", session_id, NULL);
      
      buf = g_copy_strings ( sess, "/x", NULL);
      os_x = gnome_config_get_int (buf);
      g_free(buf);
      buf = g_copy_strings ( sess, "/y", NULL);
      os_y = gnome_config_get_int (buf);
      g_free(buf);
      buf = g_copy_strings ( sess, "/w", NULL);
      os_w = gnome_config_get_int (buf);
      g_free(buf);
      buf = g_copy_strings ( sess, "/h", NULL);
      os_h = gnome_config_get_int (buf);
      g_free(buf);
    }

  return;
}

void
discard_session (gchar *id)
{
  gchar *sess;

  sess = g_copy_strings ("/gnome-hello/Saved-Session-", id, NULL);

  /* we use the gnome_config_get_* to work around a bug in gnome-config 
     (it's going under a redesign/rewrite, so i didn't correct it) */
  gnome_config_get_int ("/gnome-hello/Bug/work-around=0");

  gnome_config_clean_section (sess);
  gnome_config_sync ();

  g_free (sess);
  return;
}
