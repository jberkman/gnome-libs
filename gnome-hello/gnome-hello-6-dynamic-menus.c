/* based on :
   gnome-hello-menus.c -- Example for the "Adding menus" section
   of the Gnome Developers' Tutorial (that's is included in the
   Gnome Developers' Documentation in devel-progs/)
   */

/* Includes: Basic stuff
	     Menus
	     */

/* Copyright (C) 1998 Mark Galassi, Horacio J. Pe�a, all rights reserved */

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
void add_cb (GtkWidget *widget, void *data);
void remove_cb (GtkWidget *widget, void *data);

void prepare_app();
GtkMenuFactory *create_menu ();

GtkWidget *app;

/* The menu definitions: File/Exit and Help/About are mandatory */
GtkMenuEntry hello_menu [] =
{
  { "File/Add", NULL, (GtkMenuCallback) add_cb,  NULL },
  { "File/Remove", NULL, (GtkMenuCallback) remove_cb,  NULL },
  { "File/Exit", "<control>E", (GtkMenuCallback) quit_cb,  NULL },
  { "Changes/", NULL, NULL,  NULL },
	/* The '...' end indicate that the options open a dialog */
  { "Help/About...", "<control>A", (GtkMenuCallback) about_cb, NULL },
};

int
main(int argc, char *argv[])
{
  argp_program_version = VERSION;

  /* gnome_init() is always called at the beginning of a program.  it
     takes care of initializing both Gtk and GNOME.  It also parses
     the command-line arguments.  */
  gnome_init ("gnome-hello-1-menus", NULL, argc, argv,
	      0, NULL);

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
  app = gnome_app_new ("hello", "Hello World Gnomified");
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
  button = gtk_button_new_with_label ("Hello GNOME");
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
  g_print ("Hello GNOME\n");
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
added_item_callback (GtkWidget *widget, gpointer data)
{
  int num = (int) data;
  printf ("menu item %d\n", num);
}


void
add_cb (GtkWidget *widget, void *data)
{
  static int counter = 0;
  char buf[200];
  GnomeUIInfo entry[2];

  sprintf (buf, "menu item %d\n", counter);

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

  gnome_app_insert_menus_with_data (GNOME_APP (app),
				    "Changes/",
				    entry, NULL);

  return;
}

void
remove_cb (GtkWidget *widget, void *data)
{
  printf ("remove\n");

  gnome_app_remove_menus (GNOME_APP (app), "Changes/", 1);

  return;
}


void
about_cb (GtkWidget *widget, void *data)
{
  GtkWidget *about;
  gchar *authors[] = {
/* Here should be your names */
	  "Mark Galassi",
	  "Horacio J. Pe�a",
          NULL
          };

  about = gnome_about_new ( "The Hello World Gnomified", VERSION,
        		/* copyrigth notice */
                        "(C) 1998 the Free Software Foundation",
                        authors,
                        /* another comments */
                        "GNOME is a civilized software system "
			  "so we've a \"hello world\" program",
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

  subfactory = gtk_menu_factory_new  (GTK_MENU_FACTORY_MENU_BAR);
  gtk_menu_factory_add_entries (subfactory, hello_menu, ELEMENTS(hello_menu));

  return subfactory;
}
