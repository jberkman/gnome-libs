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

static void hello_cb (GtkWidget *widget, void *data);
static void about_cb (GtkWidget *widget, void *data);
static void quit_cb (GtkWidget *widget, void *data);

static void prepare_app(void);

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

static gboolean restarted= FALSE;
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


int
main(int argc, char *argv[])
{
  GnomeClient *client;

  /* Initialize the i18n stuff */
  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  textdomain (PACKAGE);

  gnome_init ("gnome-hello-4-SM", VERSION, argc, argv);

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
      gnome_config_push_prefix (gnome_client_get_config_prefix (client));
      
      os_x     = gnome_config_get_int  ("Geometry/x");
      os_y     = gnome_config_get_int  ("Geometry/y");
      os_w     = gnome_config_get_int  ("Geometry/w");
      os_h     = gnome_config_get_int  ("Geometry/h");
      restarted= gnome_config_get_bool ("General/restarted=0");
      
      gnome_config_pop_prefix ();
    }
  else
    {
      restarted= FALSE;
    }
  
  /* prepare_app() makes all the gtk calls necessary to set up a
     minimal Gnome application; It's based on the hello world
     example from the Gtk+ tutorial */
  prepare_app ();
  
  gtk_main ();

  return 0;
}

static void
prepare_app(void)
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
  gtk_container_set_border_width (GTK_CONTAINER (button), 60);
  gnome_app_set_contents ( GNOME_APP (app), button);

  /* We now show the widgets, the order doesn't matter, but i suggests 
     showing the main window last so the whole window will popup at
     once rather than seeing the window pop up, and then the button form
     inside of it. Although with such simple example, you'd never notice. */
  gtk_widget_show (button);
  gtk_widget_show (app);
}

/* Callbacks functions */

static void
hello_cb (GtkWidget *widget, void *data)
{
  g_print (_("Hello GNOME\n"));
  gtk_main_quit ();
  return;
}

static void
quit_cb (GtkWidget *widget, void *data)
{
  gtk_main_quit ();
  return;
}

static void
about_cb (GtkWidget *widget, void *data)
{
  GtkWidget *about;
  const gchar *authors[] = {
/* Here should be your names */
	  "Mark Galassi",
	  "Horacio J. Peña",
          NULL
          };

  about = gnome_about_new ( _("The Hello World Gnomified"), VERSION,
        		/* copyright notice */
                        "(C) 1998 the Free Software Foundation",
                        authors,
                        /* another comments */
                        _("GNOME is a civilized software system "
			  "so we've a \"hello world\" program"),
                        NULL);
  gtk_widget_show (about);

  return;
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
  gchar *prefix= gnome_client_get_config_prefix (client);
  gchar *argv[]= { "rm", "-r", NULL };
  gint x, y, w, h;

  /* The only state that gnome-hello has is the window geometry. 
     Get it. */
  gdk_window_get_geometry (app->window, &x, &y, &w, &h, NULL);

  /* Save the state using gnome-config stuff. */
  gnome_config_push_prefix (prefix);
  
  gnome_config_set_int ("Geometry/x", x);
  gnome_config_set_int ("Geometry/y", y);
  gnome_config_set_int ("Geometry/w", w);
  gnome_config_set_int ("Geometry/h", h);
  gnome_config_set_bool ("General/restarted", TRUE);

  gnome_config_pop_prefix ();
  gnome_config_sync();

  /* Here is the real SM code. We set the argv to the parameters needed
     to restart/discard the session that we've just saved and call
     the gnome_session_set_*_command to tell the session manager it. */
  argv[2]= gnome_config_get_real_path (prefix);
  gnome_client_set_discard_command (client, 3, argv);

  /* Set commands to clone and restart this application.  Note that we
     use the same values for both -- the session management code will
     automatically add whatever magic option is required to set the
     session id on startup.  */
  argv[0]= (gchar*) client_data;
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
