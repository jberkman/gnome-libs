/*
 * gnome-hello-7-mdi.c
 *
 * tries to illustrate use of GnomeMDI and GnomeMDIChild objects
 * note that toolbar creation is not demonstrated since this it is very
 * similar to menu creation. refer to gnome-mdi.[ch] and gnome-mdi-child.[ch]
 * for more details on use.
 * I hope this helps at least a bit
 *
 * Jaka Mocnik <jaka.mocnik@kiss.uni-lj.si>
 */

#include <config.h>
#include <gnome.h>

#include <stdio.h>

/* undefining this symbol would make this app use custom-built 
   menus instead of letting GnomeMDI build them from GnomeUIInfo
   templates */
#define USE_APP_HELPER 1

static void add_cb(GtkWidget *w);
static void remove_cb(GtkWidget *w);
static void add_view_cb(GtkWidget *w);
static void remove_view_cb(GtkWidget *w);
static void quit_cb(GtkWidget *w);
static void about_cb(GtkWidget *w);
static void mode_top_cb(GtkWidget *w);
static void mode_book_cb(GtkWidget *w);
static void mode_modal_cb(GtkWidget *w);

#ifdef USE_APP_HELPER
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

GnomeUIInfo main_menu[] = {
  { GNOME_APP_UI_SUBTREE, ("File"), NULL, file_menu, NULL, NULL,
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

/*
 * here we derive a new object from GnomeMDIChild. this actually belongs in a
 * separate header and source file.
 * An alternative to this would be to gtk_object_set_data() your own data to GnomeMDIChild
 * objects instead of subclassing it. This would result in more code when creating new
 * children (you'd have to create a GnomeMDIChild, set its values (name) and allocate the
 * structure with your data and then make a gtk_object_set_data() call so that this data
 * could be accessed via the GnomeMDIChild (in callbacks etc.).
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

#ifdef USE_APP_HELPER
/* the template for child-specific menus */
GnomeUIInfo child_menu[] = {
  { GNOME_APP_UI_ITEM, "Add View", NULL, add_view_cb, NULL, NULL,
    GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
  { GNOME_APP_UI_ITEM, "Remove View", NULL, remove_view_cb, NULL, NULL,
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

guint my_child_get_type () {
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
 * the create_view signal handler: creates any GtkWidget to be used as a view
 * of the child
 */
static GtkWidget *my_child_create_view(GnomeMDIChild *child) {
  GtkWidget *new_view;
  gchar label[256];

  sprintf(label, "Hello! Child %d reporting...", MY_CHILD(child)->counter);
  new_view = gtk_label_new(label);

  return new_view;
}

#ifndef USE_APP_HELPER
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

#ifndef USE_APP_HELPER
  /* if we are providing our custom-built menus instead of just
     GnomeUIInfo templates */
  child_class->create_menus = my_child_create_menus;
#endif

  parent_class = gtk_type_class (gnome_mdi_child_get_type ());
}

static void my_child_init (MyChild *child) {
#ifdef USE_APP_HELPER
  /* if we provide GnomeUIInfo menu templates */
  gnome_mdi_child_set_menu_template(GNOME_MDI_CHILD(child), main_child_menu);
#endif
}

MyChild *my_child_new(gint c) {
  MyChild *child;
  gchar name[256];

  if(child = gtk_type_new (my_child_get_type ())) {
    child->counter = c;

    /* it is ESSENTIAL that we set the GnomeMDIChild's name, otherwise some
       very strange thing might happen */
    sprintf(name, "Child #%d", c);
    GNOME_MDI_CHILD(child)->name = g_strdup(name);
  }

  return child;
}

void quit_cb (GtkWidget *widget) {
  /* when the user wants to quit we try to remove all children and if we succeed
     destroy the MDI. if TRUE was passed as the second (force) argument, remove_child
     signal wouldn't be emmited. */
  if(gnome_mdi_remove_all(mdi, FALSE))
    gtk_object_destroy(GTK_OBJECT(mdi));
}

void cleanup_cb(GnomeMDI *mdi) {
  /* on destruction of GnomeMDI we call gtk_main_quit(), since our GUI is gone */
  gtk_main_quit();
}

void add_view_cb(GtkWidget *w) {
  /* our child-menu-item activate signal handler also gets the pointer to
     the child that this menu item belongs to as the second argument */
  GnomeMDIChild *child;

  if(mdi->active_view) {
    child = VIEW_GET_CHILD(mdi->active_view);
    gnome_mdi_add_view(mdi, child);
  }
}

void remove_view_cb(GtkWidget *w) {
  /* mdi->active_view holds the pointer to the view that this action
     applies to */
  if(mdi->active_view)
    gnome_mdi_remove_view(mdi, mdi->active_view, FALSE);
}

void remove_cb(GtkWidget *w) {
  /* mdi->active_child holds the pointer to the child that this action
     applies to */
  if(mdi->active_view)
    gnome_mdi_remove_child(mdi, VIEW_GET_CHILD(mdi->active_view), FALSE);
}

void add_cb(GtkWidget *w) {
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

void mode_top_cb(GtkWidget *w) {
  gnome_mdi_set_mode(mdi, GNOME_MDI_TOPLEVEL);
}

void mode_book_cb(GtkWidget *w) {
  gnome_mdi_set_mode(mdi, GNOME_MDI_NOTEBOOK);
}

void mode_modal_cb(GtkWidget *w) {
  gnome_mdi_set_mode(mdi, GNOME_MDI_MODAL);
}

void about_cb (GtkWidget *w) {
  GtkWidget *about;
  gchar *authors[] = {
    /* Here should be your names */
    "Jaka Mocnik",
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

#ifndef USE_APP_HELPER
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
  argp_program_version = VERSION;

  /* gnome_init() is always called at the beginning of a program.  it
     takes care of initializing both Gtk and GNOME.  It also parses
     the command-line arguments.  */
  gnome_init ("gnome-hello-7-mdi", NULL, argc, argv,
	      0, NULL);
  
  mdi = GNOME_MDI(gnome_mdi_new("gnome-hello-7-mdi", "GNOME MDI Hello"));

  /* set up MDI menus: note that GnomeMDI will copy this struct for its own use, so
     main_menu[] will remain intact */
#ifdef USE_APP_HELPER
  gnome_mdi_set_menu_template(mdi, main_menu);
#else
  /* we'd use this to provide our custom-built menus */
  gtk_signal_connect(GTK_OBJECT(mdi), "create_menus", GTK_SIGNAL_FUNC(mdi_create_menus), NULL);
#endif

  /* and document menu and document list paths (see gnome-app-helper menu
     insertion routines for details)  */
  gnome_mdi_set_child_menu_path(mdi, _("File"));
  gnome_mdi_set_child_list_path(mdi, _("MDI Mode"));
  
  /* connect signals */
  gtk_signal_connect(GTK_OBJECT(mdi), "destroy", GTK_SIGNAL_FUNC(cleanup_cb), NULL);
  /* we could also connect handlers to other signals, but since we're lazy, we won't ;) */

  /* set MDI mode */
  gnome_mdi_set_mode(mdi, GNOME_MDI_NOTEBOOK);

  /* and here we go... */
  gtk_main();

  return 0;
}
