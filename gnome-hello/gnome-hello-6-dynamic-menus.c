/* based on :
   gnome-hello-menus.c -- Example for the "Adding menus" section
   of the Gnome Developers' Tutorial (that's is included in the
   Gnome Developers' Documentation in devel-progs/)
   */

/* Includes: Basic stuff
	     Menus
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
static void add_cb (GtkWidget *widget, void *data);
static void remove_cb (GtkWidget *widget, void *data);

static void prepare_app(void);

GtkWidget *app;

GnomeUIInfo file_menu[] = {
  { GNOME_APP_UI_ITEM, "Add", NULL, add_cb, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0,
    0, NULL },
  { GNOME_APP_UI_ITEM, "Remove", NULL, remove_cb, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_SEPARATOR, NULL, NULL, NULL, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, "Exit", NULL, quit_cb, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_EXIT, 'X',
    GDK_CONTROL_MASK, NULL },
  { GNOME_APP_UI_ENDOFINFO }
};

GnomeUIInfo empty_menu[] = {
  { GNOME_APP_UI_ENDOFINFO }
};

GnomeUIInfo help_menu[] = {
  { GNOME_APP_UI_ITEM, "About...", NULL, about_cb, NULL, NULL,
    GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, 0, 0, NULL },
  { GNOME_APP_UI_ENDOFINFO }
};

/* The menu definitions: File/Exit and Help/About are mandatory */
GnomeUIInfo main_menu[] = {
  { GNOME_APP_UI_SUBTREE, ("File"), NULL, file_menu, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_SUBTREE, ("Changes"), NULL, empty_menu, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_JUSTIFY_RIGHT, NULL, NULL, NULL, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_SUBTREE, ("Help"), NULL, help_menu, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_ENDOFINFO }
};

int
main(int argc, char *argv[])
{
  /* gnome_init() is always called at the beginning of a program.  it
     takes care of initializing both Gtk and GNOME.  It also parses
     the command-line arguments.  */
  gnome_init ("gnome-hello-6-dynamic-menus", VERSION, argc, argv);

  /* prepare_app() makes all the gtk calls necessary to set up a
     minimal Gnome application; It's based on the hello world example
     from the Gtk+ tutorial */
  prepare_app ();

  gtk_main ();

  return 0;
}

static void
prepare_app(void)
{
  GtkWidget *button;
  GtkMenuFactory *mf;

  /* Make the main window and binds the delete event so you can close
     the program from your WM */
  app = gnome_app_new ("hello", "Hello World Gnomified");
  gtk_widget_realize (app);
  gtk_signal_connect (GTK_OBJECT (app), "delete_event",
                      GTK_SIGNAL_FUNC (quit_cb),
                      NULL);

  /* Now that we've the main window we'll make the menues */
  gnome_app_create_menus ( GNOME_APP (app), main_menu);

  /* We make a button, bind the 'clicked' signal to hello and setting it
     to be the content of the main window */
  button = gtk_button_new_with_label ("Hello GNOME");
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
  g_print ("Hello GNOME\n");
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
added_item_callback (GtkWidget *widget, gpointer data)
{
  int num = (int) data;
  printf ("menu item %d\n", num);
}


static void
add_cb (GtkWidget *widget, void *data)
{
  static int counter = 0;
  char buf[200];
  GnomeUIInfo entry[2];
  int pos = 0;

  sprintf (buf, "menu item %d", counter);

  entry[0].type = GNOME_APP_UI_ITEM;
  entry[0].label = buf;
  entry[0].hint = NULL;
  entry[0].moreinfo = added_item_callback;
  entry[0].user_data = (gpointer) counter;
  entry[0].unused_data = NULL;
  entry[0].pixmap_type = GNOME_APP_PIXMAP_NONE;
  entry[0].pixmap_info = NULL;
  entry[0].accelerator_key = 0;
  entry[1].type = GNOME_APP_UI_ENDOFINFO;

  counter ++;

  printf ("add\n");

  gnome_app_insert_menus (GNOME_APP (app),
				    "Changes/",
				    entry);

  return;
}

static void
remove_cb (GtkWidget *widget, void *data)
{
  printf ("remove\n");

  gnome_app_remove_menus (GNOME_APP (app), "Changes/", 1);

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

  about = gnome_about_new ( "The Hello World Gnomified", VERSION,
        		/* copyright notice */
                        "(C) 1998 the Free Software Foundation",
                        authors,
                        /* another comments */
                        "GNOME is a civilized software system "
			  "so we've a \"hello world\" program",
                        NULL);
  gtk_widget_show (about);

  return;
}
