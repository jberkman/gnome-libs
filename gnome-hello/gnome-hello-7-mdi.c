/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/*
 * gnome-hello-7-mdi.c
 *
 * Illustration of use of GnomeMDI interface.
 *
 * Jaka Mocnik <jaka.mocnik@kiss.uni-lj.si>
 * Martin Baulig <martin@home-of-linux.org>
 */

#include <config.h>
#include <gnome.h>

#include <stdio.h>

/* undefining this symbol makes this app use custom-built 
   menus instead of letting GnomeMDI build them from GnomeUIInfo
   templates
*/
#define USE_TEMPLATES 1

static int restarted = 0;

static char *just_exit=NULL;
static struct poptOption prog_options[] = 
{
	{"discard-session", '\0', POPT_ARG_STRING, &just_exit, 0, N_("Discard session"), "ID"},
	POPT_AUTOHELP
	{NULL, '\0', 0, NULL, 0}
};

static int save_state (GnomeClient *, gint, GnomeRestartStyle, gint,
					   GnomeInteractStyle, gint, gpointer);

static void discard_session (gchar *);

static void add_cb(GtkWidget *w);
static void remove_cb(GtkWidget *w);
static void add_view_cb(GtkWidget *w);
static void remove_view_cb(GtkWidget *w);
static void quit_cb(GtkWidget *w);
static void about_cb(GtkWidget *w);
static void mode_top_cb(GtkWidget *w);
static void mode_book_cb(GtkWidget *w);
static void mode_modal_cb(GtkWidget *w);
static void inc_counter_cb(GtkWidget *w, gpointer user_data);

#ifdef USE_TEMPLATES
/* the template for MDI menus */
GnomeUIInfo file_menu[] = {
	{ GNOME_APP_UI_ITEM, "Add Child", NULL, add_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "Remove Child", NULL, remove_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_SEPARATOR, NULL, NULL, NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "Exit", NULL, quit_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_EXIT, 0, 0, NULL },
	{ GNOME_APP_UI_ENDOFINFO }
};

GnomeUIInfo mode_menu[] = {
	{ GNOME_APP_UI_ITEM, "Notebook", NULL, mode_book_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "Toplevel", NULL, mode_top_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "Modal", NULL, mode_modal_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_ENDOFINFO }
};

GnomeUIInfo help_menu[] = {
	{ GNOME_APP_UI_ITEM, "About...", NULL, about_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, 0, 0, NULL },
	{ GNOME_APP_UI_ENDOFINFO }
};

GnomeUIInfo empty_menu[] = {
	{ GNOME_APP_UI_ENDOFINFO }
};

GnomeUIInfo main_menu[] = {
	{ GNOME_APP_UI_SUBTREE, ("File"), NULL, file_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_SUBTREE, ("Children"), NULL, empty_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_SUBTREE, ("MDI Mode"), NULL, mode_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_JUSTIFY_RIGHT, NULL, NULL, NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_SUBTREE, ("Help"), NULL, help_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_ENDOFINFO }
};
#endif

GnomeMDI *mdi;

GnomeClient *client;

/*
 * here we derive a new object from GnomeMDIChild. this actually belongs in a
 * separate header and source file.
 * An alternative to this would be to gtk_object_set_data() your own data to GnomeMDIChild
 * objects instead of subclassing it.
 */

#define MY_CHILD(obj)          GTK_CHECK_CAST (obj, my_child_get_type (), MyChild)
#define MY_CHILD_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, my_child_get_type (), MyChildClass)
#define IS_MY_CHILD(obj)       GTK_CHECK_TYPE (obj, my_child_get_type ())

typedef struct _MyChild       MyChild;
typedef struct _MyChildClass  MyChildClass;

struct _MyChild
{
	GnomeMDIChild mdi_child;

	gint counter;
};

struct _MyChildClass
{
	GnomeMDIChildClass parent_class;
};

static GnomeMDIChildClass *parent_class = NULL;

#ifdef USE_TEMPLATES
/* the template for child-specific menus */
GnomeUIInfo child_menu[] = {
	{ GNOME_APP_UI_ITEM, "Add View", NULL, add_view_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_ITEM, "Remove View", NULL, remove_view_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_SEPARATOR },
	{ GNOME_APP_UI_ITEM, "Increase Counter", NULL, inc_counter_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },	
	{ GNOME_APP_UI_ENDOFINFO }
};

GnomeUIInfo main_child_menu[] = {
	{ GNOME_APP_UI_SUBTREE, ("Child"), NULL, child_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
	{ GNOME_APP_UI_ENDOFINFO }
};
#endif

static void my_child_class_init(MyChildClass *);
static void my_child_init(MyChild *);

static gchar *my_child_get_config_string(GnomeMDIChild *);
static GnomeMDIChild *my_child_new_from_config (const gchar *);
static GtkWidget *my_child_set_label(GnomeMDIChild *, GtkWidget *);

static guint my_child_get_type () {
	static guint my_type = 0;

	if (!my_type) {
		GtkTypeInfo my_info = {
			"MyChild",
			sizeof (MyChild),
			sizeof (MyChildClass),
			(GtkClassInitFunc) my_child_class_init,
			(GtkObjectInitFunc) my_child_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL,
		};
    
		my_type = gtk_type_unique (gnome_mdi_child_get_type (), &my_info);
	}
  
	return my_type;
}

/*
 * create_view signal handler: creates any GtkWidget to be used as a view
 * of the child
 */
static GtkWidget *my_child_create_view(GnomeMDIChild *child) {
	GtkWidget *new_view;
	gchar label[256];

	sprintf(label, "Hello! Child %d reporting...", MY_CHILD(child)->counter);
	new_view = gtk_label_new(label);

	return new_view;
}

/*
 * create config string for this child
 */
static gchar *my_child_get_config_string(GnomeMDIChild *child) {
	return g_strdup_printf ("%d", MY_CHILD(child)->counter);
}

static GtkWidget *my_child_set_label(GnomeMDIChild *child,
										  GtkWidget *old_label) {
	GtkWidget *hbox, *pixmap, *label;
	if(old_label == NULL) {
		/* if old_label is NULL, we have to create a new label */
		hbox = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new(child->name);
		gtk_widget_show(label);
		pixmap = gnome_stock_new_with_icon(GNOME_STOCK_MENU_TRASH_FULL);
		gtk_widget_show(pixmap);
		gtk_box_pack_start(GTK_BOX(hbox), pixmap, FALSE, FALSE, 2);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
	}
	else {
		hbox = old_label;
		/* old_label is a hbox we created once */
		label = GTK_WIDGET(g_list_next(gtk_container_children(GTK_CONTAINER(old_label)))->data);
		if(label)
			gtk_label_set(GTK_LABEL(label), child->name);
	}

	return hbox;
}

#ifndef USE_TEMPLATES
/* if we want to use custom-built menus for our child */
static GList *my_child_create_menus(GnomeMDIChild *child, GtkWidget *view) {
	GList *menu_list;
	GtkWidget *menu, *w;

	menu_list = NULL;

	/* the Child menu */
	menu = gtk_menu_new();

	w = gtk_menu_item_new_with_label(_("Add View"));
	gtk_signal_connect(GTK_OBJECT(w), "activate",
					   GTK_SIGNAL_FUNC(add_view_cb), child);
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);
	w = gtk_menu_item_new_with_label(_("Remove View"));
	gtk_signal_connect(GTK_OBJECT(w), "activate",
					   GTK_SIGNAL_FUNC(remove_view_cb), child);
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);

	w = gtk_menu_item_new_with_label(_("Child"));
	gtk_widget_show(w);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(w), menu);
	menu_list = g_list_append(menu_list, w);

	return menu_list;
}  
#endif

static void my_child_class_init (MyChildClass *class) {
	GtkObjectClass *object_class;
	GnomeMDIChildClass *child_class;

	object_class = (GtkObjectClass*)class;

	child_class = GNOME_MDI_CHILD_CLASS(class);

	child_class->create_view = my_child_create_view;

	/* get_config_string signal handler returns the config string for
	   MDI state restoration. you need to provide it if you intend to
	   use GnomeMDI's session saving capabilities.
	*/
	child_class->get_config_string = my_child_get_config_string;

	child_class->set_book_label = my_child_set_label;

#ifndef USE_TEMPLATES
	/* we are providing our custom-built menus instead of just
	   GnomeUIInfo templates
	*/
	child_class->create_menus = my_child_create_menus;
#endif

	parent_class = gtk_type_class (gnome_mdi_child_get_type ());
}

static void my_child_init (MyChild *child) {
#ifdef USE_TEMPLATES
	/* we provide GnomeUIInfo menu templates */
	gnome_mdi_child_set_menu_template(GNOME_MDI_CHILD(child), main_child_menu);
#endif
}

static MyChild *my_child_new(gint c) {
	MyChild *child;
	gchar name[32];

	if((child = gtk_type_new (my_child_get_type ()))) {
		child->counter = c;

		/* it is ESSENTIAL that we set the GnomeMDIChild's name, otherwise some
		   very strange things might happen */
		sprintf(name, "Child #%d", c);
		GNOME_MDI_CHILD(child)->name = g_strdup(name);
	}

	return child;
}

static GnomeMDIChild *my_child_new_from_config (const gchar *string) {
	gint c;
	
	if (sscanf (string, "%d", &c) != 1)
		return NULL;
	
	return GNOME_MDI_CHILD (my_child_new (c));
}

static void quit_cb (GtkWidget *widget) {
	/* when the user wants to quit we try to remove all children and if we
	   succeed destroy the MDI. if TRUE was passed as the second (force)
	   argument, remove_child signal wouldn't be emmited.
	*/
	if(gnome_mdi_remove_all(mdi, FALSE))
		gtk_object_destroy(GTK_OBJECT(mdi));
}

static void cleanup_cb(GnomeMDI *mdi) {
	/* on destruction of GnomeMDI we call gtk_main_quit(), since we have opened
	   no windows on our own and therefore our GUI is gone.
	*/
	gtk_main_quit();
}

static void add_view_cb(GtkWidget *w) {
	/* our child-menu-item activate signal handler also gets the pointer to
	   the child that this menu item belongs to as the second argument
	*/
	GnomeMDIChild *child;

	if(mdi->active_view) {
		child = gnome_mdi_get_child_from_view(mdi->active_view);
		gnome_mdi_add_view(mdi, child);
	}
}

static void remove_view_cb(GtkWidget *w) {
	/* mdi->active_view holds the pointer to the view that this action
	   applies to
	*/
	if(mdi->active_view)
		gnome_mdi_remove_view(mdi, mdi->active_view, FALSE);
}

static void remove_cb(GtkWidget *w) {
	/* mdi->active_child holds the pointer to the child that this action
	   applies to
	*/
	if(mdi->active_view)
		gnome_mdi_remove_child(mdi, 
							   gnome_mdi_get_child_from_view(mdi->active_view),
							   FALSE);
}

static void add_cb(GtkWidget *w) {
	MyChild *my_child;
	static gint counter = 1;

	/* create a new child */
	if((my_child = my_child_new(counter)) != NULL) {

		/* add the child to MDI */
		gnome_mdi_add_child(mdi, GNOME_MDI_CHILD(my_child));

		/* and add a new view of the child */
		gnome_mdi_add_view(mdi, GNOME_MDI_CHILD(my_child));

		counter++;
	}
}

static void mode_top_cb(GtkWidget *w) {
	gnome_mdi_set_mode(mdi, GNOME_MDI_TOPLEVEL);
}

static void mode_book_cb(GtkWidget *w) {
	gnome_mdi_set_mode(mdi, GNOME_MDI_NOTEBOOK);
}

static void mode_modal_cb(GtkWidget *w) {
	gnome_mdi_set_mode(mdi, GNOME_MDI_MODAL);
}

static void inc_counter_cb(GtkWidget *w, gpointer user_data) {
	MyChild *child = MY_CHILD(user_data);
	gchar name[32];

	child->counter++;

	sprintf(name, "Child #%d", child->counter);
	gnome_mdi_child_set_name(GNOME_MDI_CHILD(child), name);
}

static void about_cb (GtkWidget *w) {
	GtkWidget *about;
	const gchar *authors[] = {
		"Jaka Mocnik",
		"Martin Baullig",
		NULL
	};

	about = gnome_about_new ( "The Hello World Gnomified", VERSION,
							  "(C) 1998 the Free Software Foundation",
							  authors,
							  "GNOME is a civilized software system "
							  "so we've a \"hello world\" program",
							  NULL);
	gtk_widget_show (about);

	return;
}

static void reply_handler(gint reply, gpointer data) {
	gint *int_data = (gint *)data;
	*int_data = reply;
	gtk_main_quit();
}

static gint remove_child_handler(GnomeMDI *mdi, GnomeMDIChild *child) {
	gchar question[128];
	gint reply;

	sprintf(question, "Do you really want to remove child %d\n",
			MY_CHILD(child)->counter);

	gnome_app_question_modal(gnome_mdi_active_window(mdi), question,
							 reply_handler, &reply);

	/* I hope increasing main_level is the proper way to stop an app until
	   user had replied to this question... */
	gtk_main();

	if(reply == 0)
		return TRUE;

	return FALSE;
}

#ifndef USE_TEMPLATES
static GtkMenuBar *mdi_create_menus(GnomeMDI *mdi) {
	GtkWidget *menu, *w, *bar;

	bar = gtk_menu_bar_new();

	/* the File menu */
	menu = gtk_menu_new();

	w = gtk_menu_item_new_with_label(_("Add Child"));
	gtk_signal_connect(GTK_OBJECT(w), "activate",
					   GTK_SIGNAL_FUNC(add_cb), NULL);
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);

	w = gtk_menu_item_new_with_label(_("Remove Child"));
	gtk_signal_connect(GTK_OBJECT(w), "activate",
					   GTK_SIGNAL_FUNC(remove_cb), NULL);
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);

	w = gtk_menu_item_new();
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);

	w = gtk_menu_item_new_with_label(_("Exit"));
	gtk_signal_connect(GTK_OBJECT(w), "activate",
					   GTK_SIGNAL_FUNC(quit_cb), NULL);
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);

	w = gtk_menu_item_new_with_label(_("File"));
	gtk_widget_show(w);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(w), menu);

	gtk_menu_bar_append(GTK_MENU_BAR(bar), w);

	/* the Mode menu */
	menu = gtk_menu_new();

	w = gtk_menu_item_new_with_label(_("Notebook"));
	gtk_signal_connect(GTK_OBJECT(w), "activate",
					   GTK_SIGNAL_FUNC(mode_book_cb), NULL);
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);

	w = gtk_menu_item_new_with_label(_("Toplevel"));
	gtk_signal_connect(GTK_OBJECT(w), "activate",
					   GTK_SIGNAL_FUNC(mode_top_cb), NULL);
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);

	w = gtk_menu_item_new_with_label(_("Modal"));
	gtk_signal_connect(GTK_OBJECT(w), "activate",
					   GTK_SIGNAL_FUNC(mode_modal_cb), NULL);
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);

	w = gtk_menu_item_new_with_label(_("MDI Mode"));
	gtk_widget_show(w);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(w), menu);

	gtk_menu_bar_append(GTK_MENU_BAR(bar), w);

	/* the Help menu */
	menu = gtk_menu_new();

	w = gtk_menu_item_new_with_label(_("About..."));
	gtk_signal_connect(GTK_OBJECT(w), "activate",
					   GTK_SIGNAL_FUNC(about_cb), NULL);
	gtk_widget_show(w);
	gtk_menu_append(GTK_MENU(menu), w);

	w = gtk_menu_item_new_with_label(_("Help"));
	gtk_menu_item_right_justify(GTK_MENU_ITEM(w));
	gtk_widget_show(w);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(w), menu);

	gtk_menu_bar_append(GTK_MENU_BAR(bar), w);

	return GTK_MENU_BAR(bar);
}  
#endif

int main(int argc, char **argv) {
	GnomeClient *cloned;
	gboolean restart_ok = FALSE;

	/* gnome_init() is always called at the beginning of a program.  it
	   takes care of initializing both Gtk and GNOME.  It also parses
	   the command-line arguments.  */

	gnome_init_with_popt_table("gnome-hello-7-mdi",
							   VERSION, argc, argv,
							   prog_options, 0, NULL);

	/* session management init */
	client = gnome_master_client ();

	gtk_signal_connect (GTK_OBJECT (client), "save_yourself",
						GTK_SIGNAL_FUNC (save_state), NULL);

	if (just_exit) {
		discard_session(just_exit);
		return 0;
	}
  
	mdi = GNOME_MDI(gnome_mdi_new("gnome-hello-7-mdi", "GNOME MDI Hello"));

	/* set up MDI menus: note that GnomeMDI will copy this struct for its own use, so
	   main_menu[] will remain intact */
#ifdef USE_TEMPLATES
	gnome_mdi_set_menu_template(mdi, main_menu);
#else
	/* we connect the handler providing our custom-built menus */
	gtk_signal_connect(GTK_OBJECT(mdi), "create_menus", GTK_SIGNAL_FUNC(mdi_create_menus), NULL);
#endif

	/* and document menu and document list paths (see gnome-app-helper menu
	   insertion routines for details)  */
	gnome_mdi_set_child_menu_path(mdi, _("File"));
	gnome_mdi_set_child_list_path(mdi, _("Children/"));
  
	if (GNOME_CLIENT_CONNECTED (client)) {
		/* Get the client, that may hold the configuration for this
		 * program.  This client may be NULL.  That meens, that this
		 * client has not been restarted.  I.e. 'gnome_cloned_client'
		 * is only non NULL, if this application was called with the
		 * '--sm-client-id' or the '-sm-cloned-id' command line option,
		 * but this is something, the gnome libraries and the session
		 * manager take care for you.
		 */
    
		cloned = gnome_cloned_client ();
    
		if (cloned) {
			/* If cloned is non NULL, we have been restarted. */
			restarted = 1;
		}
	} else {
		/* We could even use 'gnome_client_get_config_prefix' if
		 * cloned is NULL. In this case, we would get "/gtop/" as
		 * config prefix. This is useful, if the gtop config file
		 * holds some default values for our application.
		 */
    
		cloned = NULL;
	}
	
	/* connect signals */
	gtk_signal_connect(GTK_OBJECT(mdi), "destroy",
					   GTK_SIGNAL_FUNC(cleanup_cb), NULL);
	gtk_signal_connect(GTK_OBJECT(mdi), "remove_child",
					   GTK_SIGNAL_FUNC(remove_child_handler), NULL);
	/* we could also connect handlers to other signals, but since we're lazy, we won't ;) */

	/* set MDI mode */
	gnome_mdi_set_mode(mdi, GNOME_MDI_DEFAULT_MODE);

	/* Restore MDI session. */

	if (restarted) {
		gnome_config_push_prefix
			(gnome_client_get_config_prefix (cloned));

		restart_ok = gnome_mdi_restore_state
			(mdi, "MDI Session", my_child_new_from_config);

		gnome_config_pop_prefix ();
	}

	/* and here we go... */
	gtk_main();

	return 0;
}

/* Session management */

static int save_state (GnomeClient        *client,
					   gint                phase,
					   GnomeRestartStyle   save_style,
					   gint                shutdown,
					   GnomeInteractStyle  interact_style,
					   gint                fast,
					   gpointer            client_data) {
	gchar *session_id;
	gchar *argv[3];
  
	session_id = gnome_client_get_id (client);
  
	/* Save the state using gnome-config stuff. */
	gnome_config_push_prefix (gnome_client_get_config_prefix (client));
  
	gnome_mdi_save_state (mdi, "MDI Session");
  
	gnome_config_pop_prefix();
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

static void discard_session (gchar *arg) {
	/* This discards the saved information about this client.  */
	gnome_config_clean_file (arg);
	gnome_config_sync ();
  
	/* We really need not connect, because we just exit after the
	   gnome_init call.  */
	gnome_client_disable_master_connection ();
}
