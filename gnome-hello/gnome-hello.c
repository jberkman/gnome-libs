/* gnome-hello.c -- this "hello world" style program is meant to be a
   "gnominally correct" C program */

/* Copyright (C) 1998 Mark Galassi, all rights reserved */

/* include <config.h> first.  Some stuffs in gnome.h depend on this.  */
#include <config.h>

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

  /* For internationalization, bindtextdomain() and textdomain() are needed.
     bindtextdomain() specifies where the needed message catalog files can be
     found.  textdomain selects the domain.  */
  bindtextdomain(PACKAGE, LOCALEDIR); 
  textdomain(PACKAGE); 
  
  /* prepare_app() makes all the gtk calls necessary to set up a
     minimal Gnome application; It's based on the hello world example
     from the Gtk+ tutorial */
  prepare_app(argc, argv);

  gtk_main();

  return 0;
}

void
hello_cb (GtkWidget *widget, void *data)
{
  g_print (_("Hello GNOME\n"));
  gtk_exit (0);
}

void
quit_cb (GtkWidget *widget, void *data)
{
  gtk_exit(0);
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
                        _("GNOME is a civilized software system\n"
			  "so we've a \"hello world\" program"),
                        NULL);
        gtk_widget_show (about);

        return;
}

/* The menu definitions: File/Exit and Help/About are mandatory */
GtkMenuEntry hello_menu [] = {
  { N_("File/Exit"),	 N_("<control>E"), (GtkMenuCallback) quit_cb,  NULL },
	/* The '...' end indicate that the options open a dialog */
  { N_("Help/About..."), N_("<control>A"), (GtkMenuCallback) about_cb, NULL },
};

#define ELEMENTS(x) (sizeof (x) / sizeof (x [0]))

GtkMenuFactory *
create_menu () /* taken from The Same Gnome */
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

void prepare_app(int argc, char *argv[])
{
  GtkWidget *app;
  GtkWidget *button;
  GtkMenuFactory *mf;

  /* Make the main window and binds the delete event so you can close
     the program from your WM */
  app = gnome_app_new ("hello", "Hello World Gnomified");
  gtk_widget_realize (app);
  gtk_signal_connect (GTK_OBJECT (app), "delete_event",
                      GTK_SIGNAL_FUNC (quit_cb),
                      NULL);

  /* Now that we've the main window we make the menues */
  /* I'm using GtkMenuFactory, i've asked to the gnome-list if i should
     use gnome_app_create_menu instead and i'm waiting the answer */
  mf = create_menu ();
  gnome_app_set_menus ( GNOME_APP (app), GTK_MENU_BAR (mf->widget));

  /*  gtk_container_border_width (GTK_CONTAINER (window), 10); */

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

  gtk_main ();
}
