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

void hello_cb (GtkWidget *widget, void *data);
void about_cb (GtkWidget *widget, void *data);
void quit_cb (GtkWidget *widget, void *data);

void prepare_app();
void parse_args (int argc, char *argv[]);
GtkMenuFactory *create_menu ();

static gint save_state      (GnomeClient        *client,
			     gint                phase,
			     GnomeRestartStyle   save_style,
			     gint                shutdown,
			     GnomeInteractStyle  interact_style,
			     gint                fast,
			     gpointer            client_data);

static gint die             (GnomeClient        *client,
			     gpointer            client_data);

GtkWidget *app;

int restarted = 0;
/* Old Session state (it shouldn't be global variables, but i couldn't find
   a simple better way) */
int os_x = 0,
    os_y = 0,
    os_w = 0,
    os_h = 0;

/* The menu definitions: File/Exit and Help/About are mandatory */
static GnomeUIInfo file_menu[]= {
  { 
    GNOME_APP_UI_ITEM,
    N_("Exit"), N_("Exit GNOME hello"),
    quit_cb, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_QUIT,
    'Q', GDK_CONTROL_MASK, NULL
  },
  GNOMEUIINFO_END
};

static GnomeUIInfo help_menu[]=
{
  {
    GNOME_APP_UI_ITEM,
    N_("About..."), N_("Info about GNOME hello"),
    about_cb, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT,
    0, (GdkModifierType)0, NULL
  }, 
  GNOMEUIINFO_SEPARATOR,
  GNOMEUIINFO_HELP("hello"),
  GNOMEUIINFO_END
};

static GnomeUIInfo main_menu[]= 
{
  GNOMEUIINFO_SUBTREE(N_("File"), file_menu),
  GNOMEUIINFO_SUBTREE(N_("Help"), help_menu),
  GNOMEUIINFO_END
};


/* True if parsing determined that all the work is already done.  */
int just_exit = 0;

/* These are the arguments that our application supports.  */
static struct argp_option arguments[] =
{
#define DISCARD_KEY -1
  { "discard-session", DISCARD_KEY, N_("ID"), 0, N_("Discard session"), 1 },
  { NULL, 0, NULL, 0, NULL, 0 }
};

/* Forward declaration of the function that gets called when one of
   our arguments is recognized.  */
static error_t parse_an_arg (int key, char *arg, struct argp_state *state);

/* This structure defines our parser.  It can be used to specify some
   options for how our parsing function should be called.  */
static struct argp parser =
{
  arguments,			/* Options.  */
  parse_an_arg,			/* The parser function.  */
  NULL,				/* Some docs.  */
  NULL,				/* Some more docs.  */
  NULL,				/* Child arguments -- gnome_init fills
				   this in for us.  */
  NULL,				/* Help filter.  */
  NULL				/* Translation domain; for the app it
				   can always be NULL.  */
};

int
main(int argc, char *argv[])
{
  GnomeClient *client;

  argp_program_version = VERSION;

  /* Initialize the i18n stuff */
  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  textdomain (PACKAGE);

  /* gnome_init() is always called at the beginning of a program.  it
     takes care of initializing both Gtk and GNOME.  It also parses
     the command-line arguments.  */
  gnome_init ("gnome-hello-4-SM", &parser, argc, argv,
	      0, NULL);

  /* Get the master client, that was hopefully connected to the
     session manager int the 'gnome_init' call.  All communication to
     the session manager will be done with this master client.  */
  client= gnome_master_client ();
  
  /* Arrange to be told when something interesting happens.  */
  gtk_signal_connect (GTK_OBJECT (client), "save_yourself",
		      GTK_SIGNAL_FUNC (save_state), (gpointer) argv[0]);
  gtk_signal_connect (GTK_OBJECT (client), "die",
		      GTK_SIGNAL_FUNC (die), NULL);

  if (GNOME_CLIENT_CONNECTED (client))
    {
      /* Get the client, that may hold the configuration for this
         program.  This client may be NULL.  That meens, that this
         client has not been restarted.  I.e. 'gnome_cloned_client' is
         only non NULL, if this application was called with the
         '--sm-client-id' or the '-sm-cloned-id' command line option,
         but this is something, the gnome libraries and the session
         manager take care for you.  */
      GnomeClient *cloned= gnome_cloned_client ();

      if (cloned)
	{
	  /* If cloned is non NULL, we have been restarted.  */
	  restarted= 1;

	  /* We restore information that was stored once before.  Note,
             that we use the cloned client here, because it may be
             that we are a clone of another client, which has another
             client id than we have.  */
	  gnome_config_push_prefix (gnome_client_get_config_prefix (cloned));
	  
	  os_x = gnome_config_get_int ("Geometry/x");
	  os_y = gnome_config_get_int ("Geometry/y");
	  os_w = gnome_config_get_int ("Geometry/w");
	  os_h = gnome_config_get_int ("Geometry/h");
	  
	  gnome_config_pop_prefix ();
	}
      /* We could even use 'gnome_client_get_config_prefix' is cloned
	 is NULL.  In this case, we would get "/gnome-hello-4-SM/" as
	 config prefix.  This is usefull, if the gnome-hello-4-SM
	 config file holds some default values for our application.  */
    }

  if (! just_exit)
    {
      /* prepare_app() makes all the gtk calls necessary to set up a
	 minimal Gnome application; It's based on the hello world
	 example from the Gtk+ tutorial */
      prepare_app ();

      gtk_main ();
    }

  return 0;
}

void
prepare_app()
{
  GtkWidget *button;

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
  gnome_app_create_menus (GNOME_APP (app), main_menu);

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

static error_t
parse_an_arg (int key, char *arg, struct argp_state *state)
{
  if (key == DISCARD_KEY)
    {
      /* This should discard the saved information about this client.
         Unfortunatlly is doesn't do so, yet.  */
      gnome_config_clean_file (arg);
      gnome_config_sync ();

      /* We really do not need to connect to the session manager in
         this case, because we'll just exit after the gnome_init call.  */
      gnome_client_disable_master_connection ();

      just_exit = 1;
      return 0;
    }

  /* We didn't recognize it.  */
  return ARGP_ERR_UNKNOWN;
}

/* Session management */

static gint
save_state (GnomeClient        *client,
	    gint                phase,
	    GnomeRestartStyle   save_style,
	    gint                shutdown,
	    GnomeInteractStyle  interact_style,
	    gint                fast,
	    gpointer            client_data)
{
  gchar *session_id;
  gchar *argv[3];
  gint x, y, w, h;

  session_id= gnome_client_get_id (client);

  /* The only state that gnome-hello has is the window geometry. 
     Get it. */
  gdk_window_get_geometry (app->window, &x, &y, &w, &h, NULL);

  /* Save the state using gnome-config stuff. */
  gnome_config_push_prefix (gnome_client_get_config_prefix (client));
  
  gnome_config_set_int ("Geometry/x", x);
  gnome_config_set_int ("Geometry/y", y);
  gnome_config_set_int ("Geometry/w", w);
  gnome_config_set_int ("Geometry/h", h);

  gnome_config_pop_prefix ();
  gnome_config_sync();

  /* Here is the real SM code. We set the argv to the parameters needed
     to restart/discard the session that we've just saved and call
     the gnome_session_set_*_command to tell the session manager it. */
  argv[0] = (char*) client_data;
  argv[1] = "--discard-session";
  argv[2] = gnome_client_get_config_prefix (client);
  gnome_client_set_discard_command (client, 3, argv);

  /* Set commands to clone and restart this application.  Note that we
     use the same values for both -- the session management code will
     automatically add whatever magic option is required to set the
     session id on startup.  */
  gnome_client_set_clone_command (client, 1, argv);
  gnome_client_set_restart_command (client, 1, argv);

  return TRUE;
}


static gint
die (GnomeClient        *client,
     gpointer            client_data)
{
  /* Just exit in a friendly way.  We don't need to save any state
     here, because the session manager should have sent us a
     save_yourself-message before.  */
  gtk_exit (0);

  return FALSE;
}
