/*
 * gnome-hello-menus.c -- Example for the "Adding menus" section
 * of the Gnome Developers' Tutorial (that's is included in the
 * Gnome Developers' Documentation in devel-progs/)
 *
 *Includes:
 *
 * Basic stuff
 * Menus
 * Internationalization
 * Argument Parsing.
 *
 * Copyright (C) 1998 Mark Galassi, Horacio J. Peña, all rights reserved
 */


/*
 * including gnome.h gives you all you need to use the gtk toolkit as
 * well as the GNOME libraries; it also handles internationalization
 * via GNU gettext. Including config.h before gnome.h is very important
 * (else gnome-i18n can't find ENABLE_NLS), of course i'm assuming
 * that we're in the gnome tree.
 */
#include <config.h>
#include <gnome.h>

static void prepare_app(void);

/*
 * I dont really know what arguments to use, so this is just a tmp fix
 */
static struct argp_option arguments[] = {
	{ NULL, 0, NULL, 0, NULL, 0 }
};


/* app points to our toplevel window */
GtkWidget *app;

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
				  _("(C) 1998 the Free Software Foundation"),
				  authors,
				  /* another comments */
				  _("GNOME is a civilized software system "
				    "so we've a \"hello world\" program"),
				  NULL);
	gtk_widget_show (about);
	
	return;
}

static GnomeUIInfo help_menu [] = {
	GNOMEUIINFO_ITEM_STOCK (N_("About GnomeHello..."), NULL, about_cb, GNOME_STOCK_MENU_ABOUT),
	GNOMEUIINFO_END
};

static GnomeUIInfo file_menu [] = {
	GNOMEUIINFO_ITEM_STOCK (N_("Exit"), NULL, quit_cb, GNOME_STOCK_MENU_EXIT),
	GNOMEUIINFO_END
};
	
/* The menu definitions: File/Exit and Help/About are mandatory */
static GnomeUIInfo main_menu [] = {
	GNOMEUIINFO_SUBTREE (N_("File"), &file_menu),
	GNOMEUIINFO_SUBTREE (N_("Help"), &help_menu),
	GNOMEUIINFO_END
};

/*
 * This routine parse our arguments
 */
static error_t
parse_an_arg (int key, char *arg, struct argp_state *state)
{
	if (key == 'q')
	{
		/* We found our argument.  Unfortunately, it does nothing here.
		 */
		return 0;
	}
	
	/* We didn't recognize it.  */
	return ARGP_ERR_UNKNOWN;
}



/*
 * This structure defines our parser.  It can be used to specify some
 * options for how our parsing function should be called.
 */
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
main (int argc, char *argv[])
{
	argp_program_version = VERSION;
	
	/* Initialize the i18n stuff */
	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);
	
	/*
	 * gnome_init() is always called at the beginning of a program.  it
	 * takes care of initializing both Gtk and GNOME.  It also parses
	 * the command-line arguments.
	 */
	gnome_init ("gnome-hello-1-menus", &parser, argc, argv, 0, NULL);
	
	/*
	 * prepare_app() makes all the gtk calls necessary to set up a
	 * minimal Gnome application; It's based on the hello world example
	 * from the Gtk+ tutorial
	 */
	prepare_app ();
	
	gtk_main ();
	
	return 0;
}

static void
prepare_app(void)
{
	GtkWidget *button;

	/*
	 * Make the main window and binds the delete event so you can close
	 * the program from your WM
	 */
	app = gnome_app_new ("hello", _("Hello World Gnomified"));
	gtk_signal_connect (GTK_OBJECT (app), "delete_event",
			    GTK_SIGNAL_FUNC (quit_cb),
			    NULL);
	
	/* Now that we've the main window we'll make the menues */
	gnome_app_create_menus (GNOME_APP (app), main_menu);

	/*
	 * We make a button, bind the 'clicked' signal to hello and setting it
	 * to be the content of the main window
	 */
	button = gtk_button_new_with_label (_("Hello GNOME"));
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC (hello_cb), NULL);
	gtk_container_border_width (GTK_CONTAINER (button), 60);
	gnome_app_set_contents ( GNOME_APP (app), button);

	/* We now show the widgets, the order doesn't matter, but i suggests 
	 * showing the main window last so the whole window will popup at
	 * once rather than seeing the window pop up, and then the button form
	 * inside of it. Although with such simple example, you'd never notice.
	 */
	gtk_widget_show (button);
	gtk_widget_show (app);
}


